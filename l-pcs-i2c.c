#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

/* Addresses to scan */
static const unsigned short normal_i2c[] = {
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x25, 
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x35, 
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x45, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x55, 
	I2C_CLIENT_END };


struct l_pcs_i2c_data
{
    struct i2c_client   *client;
    struct mutex        lock;
    char                valid;
    unsigned long       last_updated;
    u16                 temp;
};

static void l_pcs_i2c_update_device(struct device *dev)
{
    const int DataReadCount=2;
    
    struct l_pcs_i2c_data *data = dev_get_drvdata(dev);
    mutex_lock(&data->lock);
    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) || !data->valid)
    {
        char bytes[2];
        if (i2c_master_recv(data->client, bytes, DataReadCount) != DataReadCount)
        {
            data->temp = 0;
            data->valid = 0;
        }
        else
        {
            u16 temp = bytes[0];
            temp <<= 8;
            temp |= ((u16)bytes[1] & 0xff);
            data->temp = temp * 125;
            data->last_updated = jiffies;
            data->valid = 1;
        }
    }
    mutex_unlock(&data->lock);
}

static ssize_t temp_show(struct device *dev, struct device_attribute *da, char *buf)
{
    l_pcs_i2c_update_device(dev);
    return sprintf(buf, "%d\n", ((struct l_pcs_i2c_data *)dev_get_drvdata(dev))->temp);
}

static SENSOR_DEVICE_ATTR_RO(temp1_input, temp, 0);
static struct attribute *l_pcs_i2c_attrs[] = { &sensor_dev_attr_temp1_input.dev_attr.attr, NULL };
ATTRIBUTE_GROUPS(l_pcs_i2c);

static int l_pcs_i2c_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct l_pcs_i2c_data *data;
    struct device *hwmon_dev;
    data = devm_kzalloc(dev, sizeof(struct l_pcs_i2c_data), GFP_KERNEL);
    if (!data) 
    	return -ENOMEM;
    i2c_set_clientdata(client, data);
    mutex_init(&data->lock);
    data->client = client;
    hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name, data, l_pcs_i2c_groups);
    return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id l_pcs_i2c_ids[] = { { KBUILD_MODNAME, 0 }, { } };
MODULE_DEVICE_TABLE(i2c, l_pcs_i2c_ids);

static struct i2c_driver l_pcs_i2c_driver =
{
	.class	= I2C_CLASS_HWMON,
	.driver	= { .name = KBUILD_MODNAME },
	.probe_new	= l_pcs_i2c_probe,
	.id_table	= l_pcs_i2c_ids,
	.address_list   = normal_i2c,
};
module_i2c_driver(l_pcs_i2c_driver);

MODULE_AUTHOR("Igor Molchanov <akemi_homura@kurisa.ch>");
MODULE_DESCRIPTION("Elbrus PCS I2C access hwmon module");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");
