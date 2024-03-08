#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/gpio/driver.h>
#include <linux/slab.h>
#include <linux/types.h>

/* IO Direction Registers */
#define	MCP23016_IODIR0		0x06
#define	MCP23016_IODIR1		0x07

/* General Purpose I/O Registers */
#define	MCP23016_GP0		0x00
#define	MCP23016_GP1		0x01

/* Output Latch Registers */
#define	MCP23016_OLAT0		0x02
#define	MCP23016_OLAT1		0x03

/* Interrupt Capture Registers */
#define	MCP23016_INTCAP0	0x08
#define	MCP23016_INTCAP1	0x09

/* Expander Control Registers */
#define	MCP23016_IOCON0		0x0A
#define	MCP23016_IOCON1		0x0B

/* Default Initialization Mode */
#define	IOCON_INIT	        0x00

/* Number of GPIOs on Module */
#define GPIO_NUM            16

/* Enum of MCP Modes */
enum mcp23016_mode {
        MCP_OUTPUT_MODE,
        MCP_INPUT_MODE
};

/* MCP Struct */
struct mcp23016 {
        struct i2c_client *client;
        struct gpio_chip chip;
};

/* Get MCP Struct from a chip */
static struct mcp23016 *get_mcp23016(struct gpio_chip *chip) {
        return container_of(chip, struct mcp23016, chip);
}

/* Device ID Struct */
static const struct i2c_device_id mcp23016_id[] = {
        { "mcp23016", 0 },
        { }
};

/* Device Tree Match Struct */
static const struct of_device_id mcp23016_ids[] = {
        { .compatible = "microchip,mcp23016", },
        { }
};

static void mcp23016_write_reg8(struct i2c_client *client, u8 reg, u8 value) {
        i2c_smbus_write_byte_data(client, reg, value);
}

static u8 mcp23016_read_reg8(struct i2c_client *client, u8 reg) {
        return i2c_smbus_read_byte_data(client, reg);
}

static int mcp23016_direction(struct gpio_chip *chip, unsigned offset, int mode) {
        struct mcp23016 *mcp = get_mcp23016(chip);

        /* Calculate port and pin based on offset */
        u8 port = offset / 8;
        u8 pin = offset % 8;

        printk(KERN_INFO "Setting direction for port %d and pin %d as %s\n", port, pin, (mode == MCP_OUTPUT_MODE) ? "OUTPUT" : "INPUT");

        /* Port 0 uses reg IODIR0 and Port 1 uses IODIR1 */
        u8 reg = (port == 0) ? MCP23016_IODIR0 : MCP23016_IODIR1;

        /* Stores the actual value of GPIOs direction */
        u8 gpios_dir_val = mcp23016_read_reg8(mcp->client, reg);

        /* Sets the desired direction for the desired pin and port */
        u8 mask = 1 << pin;
        if(mode == MCP_OUTPUT_MODE)
            gpios_dir_val &= ~mask;
        else
            gpios_dir_val |= mask;

        /* Writes the new value to the IODIR register */
        mcp23016_write_reg8(mcp->client, reg, gpios_dir_val);

        return 0;
}

static int mcp23016_direction_input(struct gpio_chip *chip, unsigned offset) {
        return mcp23016_direction(chip, offset, MCP_INPUT_MODE);
}

static int mcp23016_direction_output(struct gpio_chip *chip, unsigned offset, int value) {
        return mcp23016_direction(chip, offset, MCP_OUTPUT_MODE);
}

static int mcp23016_get_value(struct gpio_chip *chip, unsigned offset) {
        struct mcp23016 *mcp = get_mcp23016(chip);

        /* Calculate port and pin based on offset */
        u8 port = offset / 8;
        u8 pin = offset % 8;

        printk(KERN_INFO "Reading value for port %d and pin %d\n", port, pin);

        /* Port 0 uses reg GP0 and Port 1 uses GP1 */
        u8 reg = (port == 0) ? MCP23016_GP0 : MCP23016_GP1;

        /* Reads the actual value of the desired pin and port */
        u8 gpios_val = mcp23016_read_reg8(mcp->client, reg);

        /* Returns the value of the desired pin */
        return (gpios_val >> pin) & 1;
}

static void mcp23016_set_value(struct gpio_chip *chip, unsigned offset, int value) {
        struct mcp23016 *mcp = get_mcp23016(chip);

        /* Calculate port and pin based on offset */
        u8 port = offset / 8;
        u8 pin = offset % 8;

        printk(KERN_INFO "Setting value for port %d and pin %d\n", port, pin);

        /* Port 0 uses reg OLAT0 and Port 1 uses OLAT1 */
        u8 reg = (port == 0) ? MCP23016_OLAT0 : MCP23016_OLAT1;

        /* Reads the actual value of the desired pin and port */
        u8 gpios_val = mcp23016_read_reg8(mcp->client, reg);

        /* Sets the desired value for the desired pin */
        if(value)
            gpios_val |= 1 << pin;
        else
            gpios_val &= ~(1 << pin);

        /* Writes the new value to the OLAT register */
        mcp23016_write_reg8(mcp->client, reg, gpios_val);
}

static int mcp23016_probe(struct i2c_client *client, const struct i2c_device_id *id) {
        struct mcp23016 *mcp;
        int ret;

        mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
        if (!mcp)
                return -ENOMEM;

        mcp->client = client;
        mcp->chip.label = client->name;
        mcp->chip.base = -1;
        mcp->chip.ngpio = GPIO_NUM;
        mcp->chip.parent = &client->dev;
        mcp->chip.owner = THIS_MODULE;
        mcp->chip.get = mcp23016_get_value;
        mcp->chip.set = mcp23016_set_value;
        mcp->chip.direction_output = mcp23016_direction_output;
        mcp->chip.direction_input = mcp23016_direction_input;
        mcp->chip.can_sleep = true;
        i2c_set_clientdata(client, mcp);

        ret = gpiochip_add(&mcp->chip);
        if(!ret)
            printk(KERN_INFO "MCP23016 GPIO driver probed\n");
        else
            printk(KERN_ERR "MCP23016 GPIO driver probe failed\n");

        return ret;
}

static void mcp23016_remove(struct i2c_client *client) {
        struct mcp23016 *mcp = i2c_get_clientdata(client);
        gpiochip_remove(&mcp->chip);
        printk(KERN_INFO "MCP23016 GPIO driver removed\n");
}

/* Register I2C driver */
struct i2c_driver i2c_drv = {
        .probe = mcp23016_probe,
        .remove = mcp23016_remove,
        .driver = {
                .owner = THIS_MODULE,
                .name = "mcp23016",
                .of_match_table = of_match_ptr(mcp23016_ids),
        },
        .id_table = mcp23016_id,
};
module_i2c_driver(i2c_drv);

MODULE_DEVICE_TABLE(i2c, mcp23016_id);
MODULE_AUTHOR("Gabriel Fonte");
MODULE_LICENSE("GPL");