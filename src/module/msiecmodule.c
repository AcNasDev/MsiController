#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/types.h>

extern int ec_read(u8 addr, u8* value);
extern int ec_write(u8 addr, u8 value);

static DEFINE_MUTEX(ec_mutex);

static ssize_t ec_raw_bin_read(struct file* filp,
                               struct kobject* kobj,
                               struct bin_attribute* attr,
                               char* buf,
                               loff_t off,
                               size_t count) {
    size_t i;
    u8 value;
    if (off >= 256)
        return 0;
    if (off + count > 256)
        count = 256 - off;
    mutex_lock(&ec_mutex);
    for (i = 0; i < count; i++) {
        if (ec_read(off + i, &value) < 0) {
            mutex_unlock(&ec_mutex);
            return -EIO;
        }
        buf[i] = value;
    }
    mutex_unlock(&ec_mutex);
    return count;
}

static ssize_t ec_raw_bin_write(struct file* filp,
                                struct kobject* kobj,
                                struct bin_attribute* attr,
                                char* buf,
                                loff_t off,
                                size_t count) {
    size_t i;
    if (off >= 256)
        return -EINVAL;
    if (off + count > 256)
        count = 256 - off;
    mutex_lock(&ec_mutex);
    for (i = 0; i < count; i++) {
        if (ec_write(off + i, buf[i]) < 0) {
            mutex_unlock(&ec_mutex);
            return -EIO;
        }
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
    pdev = platform_device_register_simple("msiec", -1, NULL, 0);
    if (IS_ERR(pdev))
        return PTR_ERR(pdev);
    ret = sysfs_create_bin_file(&pdev->dev.kobj, &ec_raw_bin_attr);
    if (ret) {
        platform_device_unregister(pdev);
        return ret;
    }
    return 0;
}

static void __exit msi_ec_raw_exit(void) {
    if (pdev) {
        sysfs_remove_bin_file(&pdev->dev.kobj, &ec_raw_bin_attr);
        platform_device_unregister(pdev);
    }
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AcNas");
MODULE_DESCRIPTION("Minimal MSI EC raw memory sysfs access (binary sysfs file)");

module_init(msi_ec_raw_init);
module_exit(msi_ec_raw_exit);