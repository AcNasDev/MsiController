#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/workqueue.h>

/* Совместимость с разными версиями ядра */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 17, 0)
#define BIN_ATTR_CONST const
#else
#define BIN_ATTR_CONST
#endif

extern int ec_read(u8 addr, u8* value);
extern int ec_write(u8 addr, u8 value);

static DEFINE_MUTEX(ec_mutex);

#define EC_MEM_SIZE 256

static u8 ec_shadow[EC_MEM_SIZE];
static u8 ec_shadow_pos;

static unsigned int cache_interval_ms = 20;
module_param(cache_interval_ms, uint, 0644);
MODULE_PARM_DESC(cache_interval_ms, "EC shadow refresh interval in ms (0 disables periodic refresh)");

static unsigned int cache_chunk = 8;
module_param(cache_chunk, uint, 0644);
MODULE_PARM_DESC(cache_chunk, "How many EC bytes to refresh per tick (1..256)");

static struct delayed_work ec_cache_work;

static void ec_cache_tick(struct work_struct* work) {
    unsigned int i;
    u8 value;
    const unsigned int interval_ms = READ_ONCE(cache_interval_ms);
    unsigned int chunk = READ_ONCE(cache_chunk);

    if (interval_ms == 0)
        return;

    if (chunk == 0)
        chunk = 1;
    if (chunk > EC_MEM_SIZE)
        chunk = EC_MEM_SIZE;

    /* Don't block if EC is busy (keyboard etc). */
    if (!mutex_trylock(&ec_mutex)) {
        schedule_delayed_work(&ec_cache_work, msecs_to_jiffies(interval_ms));
        return;
    }

    for (i = 0; i < chunk; i++) {
        const u8 addr = (u8)(ec_shadow_pos + i);
        if (ec_read(addr, &value) == 0)
            ec_shadow[addr] = value;
    }
    ec_shadow_pos = (u8)(ec_shadow_pos + chunk);
    mutex_unlock(&ec_mutex);

    schedule_delayed_work(&ec_cache_work, msecs_to_jiffies(interval_ms));
}

static void ec_shadow_full_refresh(void) {
    int i;
    u8 value;

    mutex_lock(&ec_mutex);
    for (i = 0; i < EC_MEM_SIZE; i++) {
        if (ec_read((u8)i, &value) == 0)
            ec_shadow[i] = value;
    }
    mutex_unlock(&ec_mutex);
}

static ssize_t ec_raw_bin_read(struct file* filp,
                               struct kobject* kobj,
                               BIN_ATTR_CONST struct bin_attribute* attr,
                               char* buf,
                               loff_t off,
                               size_t count) {
    if (off < 0)
        return -EINVAL;
    if (off >= EC_MEM_SIZE)
        return 0;
    if (count > EC_MEM_SIZE - (size_t)off)
        count = EC_MEM_SIZE - (size_t)off;

    /* Return a consistent cached snapshot without new EC transactions. */
    mutex_lock(&ec_mutex);
    memcpy(buf, ec_shadow + off, count);
    mutex_unlock(&ec_mutex);
    return count;
}

static ssize_t ec_raw_bin_write(struct file* filp,
                                struct kobject* kobj,
                                BIN_ATTR_CONST struct bin_attribute* attr,
                                char* buf,
                                loff_t off,
                                size_t count) {
    size_t i;
    if (off < 0 || off >= EC_MEM_SIZE)
        return -EINVAL;
    if (count > EC_MEM_SIZE - (size_t)off)
        count = EC_MEM_SIZE - (size_t)off;
    mutex_lock(&ec_mutex);
    for (i = 0; i < count; i++) {
        if (ec_write(off + i, buf[i]) < 0) {
            mutex_unlock(&ec_mutex);
            return -EIO;
        }
        ec_shadow[off + i] = buf[i];
    }
    mutex_unlock(&ec_mutex);
    return count;
}

static struct bin_attribute ec_raw_bin_attr = {
    .attr =
        {
            .name = "ec",
            .mode = 0660,
        },
    .size = 256,
    .read = ec_raw_bin_read,
    .write = ec_raw_bin_write,
};

static struct platform_device* pdev;

static int __init msi_ec_raw_init(void) {
    int ret;

    memset(ec_shadow, 0, sizeof(ec_shadow));
    ec_shadow_pos = 0;

    pdev = platform_device_register_simple("msiec", -1, NULL, 0);
    if (IS_ERR(pdev))
        return PTR_ERR(pdev);
    ret = sysfs_create_bin_file(&pdev->dev.kobj, &ec_raw_bin_attr);
    if (ret) {
        platform_device_unregister(pdev);
        return ret;
    }

    /* One-time full refresh so userspace immediately sees sane data. */
    ec_shadow_full_refresh();

    INIT_DELAYED_WORK(&ec_cache_work, ec_cache_tick);
    if (cache_interval_ms != 0)
        schedule_delayed_work(&ec_cache_work, msecs_to_jiffies(cache_interval_ms));
    return 0;
}

static void __exit msi_ec_raw_exit(void) {
    if (pdev) {
        cancel_delayed_work_sync(&ec_cache_work);
        sysfs_remove_bin_file(&pdev->dev.kobj, &ec_raw_bin_attr);
        platform_device_unregister(pdev);
    }
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AcNas");
MODULE_DESCRIPTION("Minimal MSI EC raw memory sysfs access (binary sysfs file)");

module_init(msi_ec_raw_init);
module_exit(msi_ec_raw_exit);