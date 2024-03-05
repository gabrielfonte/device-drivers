#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
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

/* Enum of MCP Modes */
enum mcp23016_mode {
        MCP_OUTPUT_MODE,
        MCP_INPUT_MODE
};

/* Enum of MCP Pins */
enum mcp23016_pin {
        MCP_GPIO_PIN0,
        MCP_GPIO_PIN1,
        MCP_GPIO_PIN2,
        MCP_GPIO_PIN3,
        MCP_GPIO_PIN4,
        MCP_GPIO_PIN5,
        MCP_GPIO_PIN6,
        MCP_GPIO_PIN7
};

/* Enum of MCP Pin State */
enum mcp23016_pin_state {
        MCP_PIN_LOW,
        MCP_PIN_HIGH
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

static void mcp23016_write_reg8(struct i2c_client *client, u8 reg, u8 value) {
        i2c_smbus_write_byte_data(client, reg, value);
}

static u8 mcp23016_read_reg8(struct i2c_client *client, u8 reg) {
        return i2c_smbus_read_byte_data(client, reg);
}

static int mcp23016_direction_input(struct gpio_chip *chip, unsigned offset) {
        struct mcp23016 *mcp = get_mcp23016(chip);
        u8 reg = (offset < 8) ? MCP23016_IODIR0 : MCP23016_IODIR1;
        u8 mask = 1 << (offset & 7);
        u8 val = mcp23016_read_reg8(mcp->client, reg);
        val |= mask;
        mcp23016_write_reg8(mcp->client, reg, val);
        return 0;
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
        mcp->chip.ngpio = 16;
        mcp->chip.parent = &client->dev;
        mcp->chip.owner = THIS_MODULE;
        mcp->chip.request = gpiochip_generic_request;
        mcp->chip.free = gpiochip_generic_free;
        mcp->chip.get = gpiochip_generic_get;
        mcp->chip.set = gpiochip_generic_set;
        mcp->chip.direction_output = gpiochip_generic_direction_output;
        mcp->chip.direction_input = gpiochip_generic_direction_input;
        mcp->chip.can_sleep = true;
        i2c_set_clientdata(client, mcp);

        ret = devm_gpiochip_add_data(&client->dev, &mcp->chip, mcp);
        if (ret)
            return ret;

        return 0;
}

/* Register I2C driver */
static struct i2c_driver i2c_drv = {
        .driver = {
                .owner = THIS_MODULE,
                .name = "mcp23016",
                .of_match_table = of_match_ptr(mcp23016_of_match),
        },
        .probe = mcp23016_probe,
        .remove = mcp23016_remove,
        .id_table = mcp23016_id,
};
module_i2c_driver(i2c_drv);

MODULE_DEVICE_TABLE(i2c, mcp23016_id);
MODULE_AUTHOR("Gabriel Fonte");
MODULE_LICENSE("GPL");