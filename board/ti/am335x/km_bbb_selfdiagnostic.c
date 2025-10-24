#include <common.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <dm.h>
#include <edid.h>
#include <env_internal.h>
#include <errno.h>
#include <i2c.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include <km_char_lcd.h>
#include <string.h>
#include <asm/gpio.h>
#include <dm/device-internal.h>

/* KM-BBB Headers */
#include <km_bbb_bootenv.h>

void gpio_input_testcase(int ,int );
void gpio_output_testcase(int );
static int spi_testcase(void);
static int i2c_scan (void);
static int i2c_set_bus_num(unsigned int);
static int i2c_get_cur_bus(struct udevice **);


void All_Test_Cases(void){
	int ret;

#ifdef CONFIG_KM_BBB_LED_SWITCHES
	printf("gpio output testcases - LED & Buzzer\n");
	HD44780_ClrScr();
	HD44780_Str_XY(0,0,"Self Diagnostic");
	HD44780_Str_XY(0,1,"LED & Buzzer");
	gpio_output_testcase(10);
	gpio_output_testcase(9);
	gpio_output_testcase(10);
	gpio_output_testcase(9);

	printf("gpio input testcases - Switches\n");
	printf("By default ENTER  Sw configure to Pull Down & remaining switches configure to Pull up\n");
	printf("Press ENTER Switch\n");
	HD44780_Str_XY(0,1,"                ");
	HD44780_Str_XY(0,1,"Press ENTER Sw");
	gpio_input_testcase(11,0);

	printf("Press DOWN Switch\n");
	HD44780_Str_XY(0,1,"                ");
	HD44780_Str_XY(0,1,"Press DOWN Sw");
	gpio_input_testcase(27,1);

	printf("Press UP Switch\n");
	HD44780_Str_XY(0,1,"                ");
	HD44780_Str_XY(0,1,"Press UP Sw");
	gpio_input_testcase(26,1);       // gpio pin number is the first argument By Default Value is 0
#endif

	printf("I2C Test Cases: I2C1 Scaning ...\n");
	HD44780_Str_XY(0,1,"                ");
	HD44780_Str_XY(0,1,"I2C Scanning");
	ret = i2c_set_bus_num(1); // set i2c dev 1
	if (ret)
		printf("Failure changing bus number (%d)\n", ret);
	i2c_scan();

#ifdef CONFIG_KM_BBB_ENC28J60
	printf("SPI Test Case: ENC28J60 Test ...\n");
	HD44780_Str_XY(0,1,"                ");
	HD44780_Str_XY(0,1,"SPI2Eth Test");
	ret = spi_testcase();
	if (ret != 0) {
		printf("SPI Test Case FAIL\n");
		HD44780_Str_XY(0,1,"SPI2Eth Test FAIL");
	} else {
		printf("SPI Test Case PASS: ret:%d\n",ret);
		HD44780_Str_XY(0,1,"                ");
		HD44780_Str_XY(0,1,"SPI2Eth Test PASS");
	}
#endif

	HD44780_ClrScr();
        HD44780_Str_XY(0,0,"All Tests DONE");
	printf("All Tests COMPLETE\n");
	printf("To enter bootmenu, run \"km_bootmenu\" command\n");
}

void gpio_output_testcase(int gpio)
{
	int ret;
	ret = gpio_request(gpio, "cmd_gpio");
        if (ret && ret != -EBUSY) {
                printf("gpio: requesting pin %u failed\n", 11);
        }
        gpio_direction_output(gpio,1);
        Delay_HD44780(100);
        gpio_direction_output(gpio,0);
        Delay_HD44780(100);
}

void gpio_input_testcase(int gpio,int status)
{
       int ret,value=100,Ent=0;

	ret = gpio_request(gpio, "cmd_gpio");
        if (ret && ret != -EBUSY) {
                printf("gpio: requesting pin %u failed\n", 11);
        }
        gpio_direction_input(gpio);
        value =gpio_get_value(gpio);
        while(value==status) {
                value = gpio_get_value(gpio);
                if(value==(!status)){
			Ent=1;
			break;
                }
	}

	if(Ent==1) {
                printf("Ent:%d\n",Ent);
		gpio_direction_output(9,1);
		Delay_HD44780(100);
		gpio_direction_output(9,0);
		Delay_HD44780(100);
	}
	Ent=0;
}

static int spi_testcase(void)
{
	int ret;
	run_command_list(KM_SPI_ENV, -1, 0); // Setup SPI Environment
	ret = run_command_list(KM_SPI_TEST, -1, 0);// Test Ping Command
	return ret;
}


#if CONFIG_IS_ENABLED(DM_I2C)
static struct udevice *i2c_cur_bus;

static int i2c_set_bus_num(unsigned int busnum)
{
	struct udevice *bus;
	int ret;

	ret = uclass_get_device_by_seq(UCLASS_I2C, busnum, &bus);
	if (ret) {
		debug("%s: No bus %d\n", __func__, busnum);
		return ret;
	}
	i2c_cur_bus = bus;

	return 0;
}

static int i2c_get_cur_bus(struct udevice **busp)
{
#ifdef CONFIG_I2C_SET_DEFAULT_BUS_NUM
	if (!i2c_cur_bus) {
		if (i2c_set_bus_num(CONFIG_I2C_DEFAULT_BUS_NUMBER)) {
			printf("Default I2C bus %d not found\n",
			       CONFIG_I2C_DEFAULT_BUS_NUMBER);
			return -ENODEV;
		}
	}
#endif

	if (!i2c_cur_bus) {
		puts("No I2C bus selected\n");
		return -ENODEV;
	}
	*busp = i2c_cur_bus;

	return 0;
}
#endif

static int i2c_scan (void)
{
        int j;
        int found = 0;
        char str[10];
#if defined(CONFIG_SYS_I2C_NOPROBES)
        int k, skip;
        unsigned int bus =1;
#endif  /* NOPROBES */
        int ret;
#ifdef CONFIG_DM_I2C
        struct udevice *bus, *dev;

        if (i2c_get_cur_bus(&bus))
                return CMD_RET_FAILURE;
#endif

        puts ("Valid chip addresses:\n");
        for (j = 0; j < 128; j++) {

#if defined(CONFIG_SYS_I2C_NOPROBES)
                skip = 0;
                for (k = 0; k < ARRAY_SIZE(i2c_no_probes); k++) {
                        if (COMPARE_BUS(bus, k) && COMPARE_ADDR(j, k)) {
                                skip = 1;
                                break;
                        }
                }
                if (skip)
                        continue;
#endif
#ifdef CONFIG_DM_I2C
                ret = dm_i2c_probe(bus, j, 0, &dev);
#else
                ret = i2c_probe(j);
#endif
		if(ret==0){
			if (j==80)
				printf("EEPROM:%d\n",j);
			else if (j==83)
				printf("ADXL345 - Accelerometer Sensor:%d\n",j);
			else if (j==104)
				printf("DS1307-RTC:%d\n",j);
			else
	                        printf("Found I2C device: %02X\n", j);
                        found++;
			sprintf(str,"%d",j);
			HD44780_Str_XY(0,1,"Device Found:");
			HD44780_Str_XY(13,1,str);
			Delay_HD44780(100);
                }
        }
        putc ('\n');

#if defined(CONFIG_SYS_I2C_NOPROBES)
        puts ("Excluded chip addresses:");
        for (k = 0; k < ARRAY_SIZE(i2c_no_probes); k++) {
                if (COMPARE_BUS(bus,k))
                        printf(" %02X", NO_PROBE_ADDR(k));
        }
        putc ('\n');
#endif

        return (0 == found);
}
