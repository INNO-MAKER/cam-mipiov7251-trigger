/*
 * Driver for InnoMaker Co.,ltd  MIPI ov7251 Sensor module - CMOS Image Sensor from Omnivision
 *
 * Copyright (C) 2022, Jack Yang <jack@inno-maker.com>
 *
 * based on 
 *
 * IMX219 driver, Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * Copyright (C) 2014, Andrew Chew <achew@nvidia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-mediabus.h>
#include <linux/version.h>

/* OV7251 supported geometry */
#define OV7251_TABLE_END		0xffff

/* In real */
#define OV7251_DIGITAL_GAIN_MIN		0x00
#define OV7251_DIGITAL_GAIN_MAX		0x3FF
#define OV7251_DIGITAL_GAIN_DEFAULT	0x10

#define OV7251_DIGITAL_EXPOSURE_MIN	    1
#define OV7251_DIGITAL_EXPOSURE_MAX	    496
#define OV7251_DIGITAL_EXPOSURE_DEFAULT	    400


#define OV7251_SC_MODE_SELECT		0x0100
#define OV7251_SC_MODE_SELECT_SW_STANDBY	0x0
#define OV7251_SC_MODE_SELECT_STREAMING		0x1




#define OV7251_CHIP_ID_HIGH		0x300a
#define OV7251_CHIP_ID_HIGH_BYTE	0x77
#define OV7251_CHIP_ID_LOW		0x300b
#define OV7251_CHIP_ID_LOW_BYTE		0x50
#define OV7251_SC_GP_IO_IN1		0x3029
#define OV7251_AEC_EXPO_0		0x3500
#define OV7251_AEC_EXPO_1		0x3501
#define OV7251_AEC_EXPO_2		0x3502
#define OV7251_AEC_AGC_ADJ_0		0x350a
#define OV7251_AEC_AGC_ADJ_1		0x350b
/* Exposure must be at least 20 lines shorter than VTS */
#define OV7251_EXPOSURE_OFFSET		20
 /* HTS is registers 0x380c and 0x380d */
#define OV7251_HTS			0x3a0
#define OV7251_VTS_HIGH			0x380e
#define OV7251_VTS_LOW			0x380f
#define OV7251_VTS_MIN_OFFSET		92
#define OV7251_VTS_MAX			0x7fff
#define OV7251_TIMING_FORMAT1		0x3820
#define OV7251_TIMING_FORMAT1_VFLIP	BIT(2)
#define OV7251_TIMING_FORMAT2		0x3821
#define OV7251_TIMING_FORMAT2_MIRROR	BIT(2)
#define OV7251_PRE_ISP_00		0x5e00
#define OV7251_PRE_ISP_00_TEST_PATTERN	BIT(7)
#define OV7251_PLL1_PRE_DIV_REG		0x30b4
#define OV7251_PLL1_MULT_REG		0x30b3
#define OV7251_PLL1_DIVIDER_REG		0x30b1
#define OV7251_PLL1_PIX_DIV_REG		0x30b0
#define OV7251_PLL1_MIPI_DIV_REG	0x30b5
#define OV7251_PLL2_PRE_DIV_REG		0x3098
#define OV7251_PLL2_MULT_REG		0x3099
#define OV7251_PLL2_DIVIDER_REG		0x309d
#define OV7251_PLL2_SYS_DIV_REG		0x309a
#define OV7251_PLL2_ADC_DIV_REG		0x309b

/*
 * OV7251 native and active pixel array size.
 * Datasheet not available to confirm these values, so assume there are no
 * border pixels.
 */
#define OV7251_NATIVE_WIDTH		    640U
#define OV7251_NATIVE_HEIGHT		480U
#define OV7251_PIXEL_ARRAY_LEFT		0U
#define OV7251_PIXEL_ARRAY_TOP		0U
#define OV7251_PIXEL_ARRAY_WIDTH	640U
#define OV7251_PIXEL_ARRAY_HEIGHT	480U

#define OV7251_PIXEL_CLOCK 48000000



/*Sensor work Mode - default 8-Bit Streaming */
static int sensor_mode = 1;
module_param(sensor_mode, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(sensor_mode, "Sensor work Mode: 0=10bit_stream 1=8bit_stream  2=10bit_tigger 3=8bit_tigger  ");

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x60, 0x60 , I2C_CLIENT_END };

static const s64 link_freq_menu_items[] = {
	800000000,
};

struct ov7251_reg {
	u16 addr;
	u8 val;
};

struct ov7251_mode {
	u32 sensor_mode;
	u32 sensor_depth;
	u32 sensor_ext_trig;
	u32 width;
	u32 height;
	u32 max_fps;
	u32 hts_def;
	u32 vts_def;
	const struct ov7251_reg *reg_list;
};

/*
 * Xclk 24Mhz
 * max_framerate 150fps
 * mipi_datarate per lane 800Mbps
 */
static const struct ov7251_reg  ov7251_setting_full_vga_10_183fps[] = {
	
	{OV7251_TABLE_END, 0x00},
};
static const struct ov7251_reg ov7251_setting_full_vga_8_183fps[] = {
	
        {OV7251_TABLE_END, 0x00},
};


static const struct ov7251_reg start[] = {
	{OV7251_SC_MODE_SELECT, OV7251_SC_MODE_SELECT_STREAMING},/* mode select streaming on */
	{OV7251_TABLE_END, 0x00}
};

static const struct ov7251_reg stop[] = {
	{OV7251_SC_MODE_SELECT, OV7251_SC_MODE_SELECT_SW_STANDBY},/* mode select streaming off */
	{OV7251_TABLE_END, 0x00}
};


#define SIZEOF_I2C_TRANSBUF 32

struct inno_rom_table {
	char magic[12];
	char manuf[32];
	u16 manuf_id;
	char sen_manuf[8];
	char sen_type[16];
	u16 mod_id;
	u16 mod_rev;
	char regs[56];
	u16 nr_modes;
	u16 bytes_per_mode;
	char mode1[16];
	char mode2[16];
};

struct ov7251 {
	struct v4l2_subdev subdev;
	struct media_pad pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct clk *clk;
	int hflip;
	int vflip;
	u16 digital_gain;
	u32 exposure_time;
	struct v4l2_ctrl *pixel_rate;
	const struct ov7251_mode *cur_mode;
	struct i2c_client *rom;
	struct inno_rom_table rom_table;
};

static const struct ov7251_mode supported_modes[] = {
	{
		.sensor_mode = 0,
		.sensor_ext_trig = 0,
		.sensor_depth = 10,
		.width = 640,
		.height = 480,
		.max_fps = 120,
		.hts_def = 772,
		.vts_def = 0x23c,
		.reg_list = ov7251_setting_full_vga_10_183fps,
		
	},
	
	{
		.sensor_mode = 1,
		.sensor_ext_trig = 0,
		.sensor_depth = 8,
		.width = 640,
		.height = 480,
		.max_fps = 120,
		.hts_def = 772,
		.vts_def = 0x23c,
		.reg_list = ov7251_setting_full_vga_8_183fps,
		
	},
	
	{
		.sensor_mode = 2,
		.sensor_ext_trig = 1,
		.sensor_depth = 10,
		.width = 640,
		.height = 480,
		.max_fps = 120,
		.hts_def = 772,
		.vts_def = 0x23c, 
		.reg_list = ov7251_setting_full_vga_10_183fps,
	},
		{
		.sensor_mode = 3,
		.sensor_ext_trig = 1,
		.sensor_depth = 8,
		.width = 640,
		.height = 480,
		.max_fps = 120,
		.hts_def = 772,
		.vts_def = 0x23c,
		.reg_list = ov7251_setting_full_vga_8_183fps,
	},
	
}; 

static struct ov7251 *to_ov7251(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov7251, subdev);
}

static int reg_write(struct i2c_client *client, const u16 addr, const u8 data)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	u8 tx[3];
	int ret;

	msg.addr = client->addr;
	msg.buf = tx;
	msg.len = 3;
	msg.flags = 0;
	tx[0] = addr >> 8;
	tx[1] = addr & 0xff;
	tx[2] = data;
	ret = i2c_transfer(adap, &msg, 1);
	mdelay(2);

	return ret == 1 ? 0 : -EIO;
}

static int rom_write(struct i2c_client *client, const u16 addr, const u8 data)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	u8 tx[2];
	int ret;

	msg.addr = client->addr;
	msg.buf = tx;
	msg.len = 2;
	msg.flags = 0;
	tx[0] = addr ;
	tx[1] = data;	
	ret = i2c_transfer(adap, &msg, 1);
	mdelay(2);

	return ret == 1 ? 0 : -EIO;
}

static int reg_read(struct i2c_client *client, const u16 addr)
{
	u8 buf[2] = {addr >> 8, addr & 0xff};
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr  = client->addr,
			.flags = 0,
			.len   = 2,
			.buf   = buf,
		}, {
			.addr  = client->addr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = buf,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_warn(&client->dev, "Reading register %x from %x failed\n",
			 addr, client->addr);
		return ret;
	}

	return buf[0];
}

static int rom_read(struct i2c_client *client, const u16 addr)
{
	u8 buf[1] = {addr};
	int ret;
	struct i2c_msg msgs[] = {
		{
			.addr  = client->addr,
			.flags = 0,
			.len   = 1,
			.buf   = buf,
		}, {
			.addr  = client->addr,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = buf,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_warn(&client->dev, "Reading register %x from %x failed\n",
			 addr, client->addr);
		return ret;
	}

	return buf[0];
}

static int reg_write_table(struct i2c_client *client,
			   const struct ov7251_reg table[])
{
	const struct ov7251_reg *reg;
	int ret;

	for (reg = table; reg->addr != OV7251_TABLE_END; reg++) {
		ret = reg_write(client, reg->addr, reg->val);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* V4L2 subdev video operations */
static int ov7251_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7251 *priv = to_ov7251(client);
	int ret;
	int addr;
	int data;

	if (!enable)
		return reg_write_table(client, stop);

	ret = reg_write_table(client, priv->cur_mode->reg_list);//set mode register
	if (ret)
		return ret;
	
	if(priv->cur_mode->sensor_ext_trig)
	{		
                         addr = 208; 
		         data = 1; // ext trig enable
			 ret = rom_write(priv->rom, addr, data);	
                         
                         mdelay(10);		
	}	
	else
	{
	                 addr = 208; 
		         data = 0; // ext trig disable
			 ret = rom_write(priv->rom, addr, data);	
                         
                         mdelay(5);		
	}
	
	return reg_write_table(client, start);
}

/* V4L2 subdev core operations */
static int ov7251_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7251 *priv = to_ov7251(client);

	if (on)	{
		dev_dbg(&client->dev, "ov7251 power on\n");
		clk_prepare_enable(priv->clk);
	} else if (!on) {
		dev_dbg(&client->dev, "ov7251 power off\n");
		clk_disable_unprepare(priv->clk);
	}

	return 0;
}

static int ov7251_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov7251 *priv =
	    container_of(ctrl->handler, struct ov7251, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&priv->subdev);
	u8 reg;
	int ret;
	u16 gain = 0;
	u32 exposure = 0;

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		priv->hflip = ctrl->val;
		if(ctrl->val)
		 ret=reg_write(client, OV7251_TIMING_FORMAT2,0x04 );
	        else
		 ret=reg_write(client, OV7251_TIMING_FORMAT2,  0x00);		
             return ret;    
	case V4L2_CID_VFLIP:
		priv->vflip = ctrl->val;
		if(ctrl->val)
		 ret=reg_write(client, OV7251_TIMING_FORMAT1, 0x04);
	        else		 
		 ret=reg_write(client, OV7251_TIMING_FORMAT1, 0x40);		
       	     return ret;
	case V4L2_CID_GAIN:

		gain = ctrl->val;

		if (gain < OV7251_DIGITAL_GAIN_MIN) 
			gain = OV7251_DIGITAL_GAIN_MIN;
		if (gain > OV7251_DIGITAL_GAIN_MAX)
			gain = OV7251_DIGITAL_GAIN_MAX;
		
		priv->digital_gain = gain;
		dev_info(&client->dev, "GAIN = %d \n",gain);

		ret  = reg_write(client, OV7251_AEC_AGC_ADJ_0, (gain & 0x0300) >> 8);/* goes to OV7251_AEC_AGC_ADJ_0 */
		ret |= reg_write(client, OV7251_AEC_AGC_ADJ_1, gain & 0xff);

		return ret;

	case V4L2_CID_EXPOSURE:

		exposure = ctrl->val ; 
		
		if (exposure < OV7251_DIGITAL_EXPOSURE_MIN)
			exposure = OV7251_DIGITAL_EXPOSURE_MIN;
		if (exposure > OV7251_DIGITAL_EXPOSURE_MAX)
			exposure = OV7251_DIGITAL_EXPOSURE_MAX;
		
		priv->exposure_time = exposure ;

		dev_info(&client->dev, "EXPOSURE = %d \n",exposure);

		ret  = reg_write(client, OV7251_AEC_EXPO_0, (exposure & 0xf000) >> 12);
		ret |= reg_write(client, OV7251_AEC_EXPO_1, (exposure & 0x0ff0) >> 4);
		ret |= reg_write(client, OV7251_AEC_EXPO_2,  (exposure & 0x000f) << 4);

		return ret;

	default:
		return -EINVAL;
	}

	/* If enabled, apply settings immediately */
	reg = reg_read(client, 0x100);
	if ((reg & 0x1f) == 0x01)
		ov7251_s_stream(&priv->subdev, 1);

	return 0;
}

static int ov7251_enum_mbus_code(struct v4l2_subdev *sd,
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,15,0) 
				 struct v4l2_subdev_state *sd_state,
#else
				 struct v4l2_subdev_pad_config *cfg,
#endif			 
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7251 *priv = to_ov7251(client);
	const struct ov7251_mode *mode = priv->cur_mode;

	if (code->index !=0)
		return -EINVAL;
	
	if(mode->sensor_depth==8)
		code->code = MEDIA_BUS_FMT_Y8_1X8;

	if(mode->sensor_depth==10)
		code->code = MEDIA_BUS_FMT_Y10_1X10;

	return 0;
}


static const struct ov7251_mode *ov7251_find_best_fit(
					struct v4l2_subdev_format *fmt)
{

	return &supported_modes[sensor_mode];

}

static int ov7251_set_fmt(struct v4l2_subdev *sd,
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,15,0) 
			  struct v4l2_subdev_state *sd_state,
#else			  
			  struct v4l2_subdev_pad_config *cfg,
#endif		  
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7251 *priv = to_ov7251(client);
	const struct ov7251_mode *mode;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	mode = ov7251_find_best_fit(fmt);
	if(mode->sensor_depth==8)
		fmt->format.code = MEDIA_BUS_FMT_Y8_1X8;
	if(mode->sensor_depth==10)
		fmt->format.code = MEDIA_BUS_FMT_Y10_1X10;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	priv->cur_mode = mode;

	return 0;
}

static int ov7251_get_fmt(struct v4l2_subdev *sd,
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,15,0) 
			  struct v4l2_subdev_state *sd_state,
#else			  
			  struct v4l2_subdev_pad_config *cfg,
#endif		  
			  struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7251 *priv = to_ov7251(client);
	const struct ov7251_mode *mode = priv->cur_mode;
	s64 pixel_rate;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	if(mode->sensor_depth==8)
		fmt->format.code = MEDIA_BUS_FMT_Y8_1X8;
	if(mode->sensor_depth==10)
		fmt->format.code = MEDIA_BUS_FMT_Y10_1X10;
	fmt->format.field = V4L2_FIELD_NONE;

	pixel_rate = mode->vts_def * mode->hts_def * mode->max_fps;
	__v4l2_ctrl_modify_range(priv->pixel_rate, pixel_rate,
					pixel_rate, 1, pixel_rate);

	return 0;
}

/* Various V4L2 operations tables */
static struct v4l2_subdev_video_ops ov7251_subdev_video_ops = {
	.s_stream = ov7251_s_stream,
};

static struct v4l2_subdev_core_ops ov7251_subdev_core_ops = {
	.s_power = ov7251_s_power,
};

static const struct v4l2_subdev_pad_ops ov7251_subdev_pad_ops = {
	.enum_mbus_code = ov7251_enum_mbus_code,
	.set_fmt = ov7251_set_fmt,
	.get_fmt = ov7251_get_fmt,
};

static struct v4l2_subdev_ops ov7251_subdev_ops = {
	.core = &ov7251_subdev_core_ops,
	.video = &ov7251_subdev_video_ops,
	.pad = &ov7251_subdev_pad_ops,
};

static const struct v4l2_ctrl_ops ov7251_ctrl_ops = {
	.s_ctrl = ov7251_s_ctrl,
};

static int ov7251_video_probe(struct i2c_client *client)
{
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	u16 model_id;
	u32 lot_id=0;
	u16 chip_id=0;
	int ret;

	ret = ov7251_s_power(subdev, 1);
	if (ret < 0)
		return ret;

	/* Check and show model, lot, and chip ID. */
	ret = reg_read(client, OV7251_CHIP_ID_HIGH);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Model ID (high byte)\n");
		goto done;
	}
	model_id = ret << 8; 

	ret = reg_read(client, OV7251_CHIP_ID_LOW);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Model ID (low byte)\n");
		goto done;
	}
	model_id |= ret;
	
	ret = reg_read(client, OV7251_SC_GP_IO_IN1);
	if (ret < 0) {
		dev_err(&client->dev, "Failure to read Lot ID (low byte)\n");
		goto done;
	}
	lot_id = ret;

	dev_info(&client->dev,
		 "Model ID 0x%04x, Lot ID 0x%06x, Chip ID 0x%04x\n",
		 model_id, lot_id, chip_id);
done:
	ov7251_s_power(subdev, 0);
	return ret;
}

static int ov7251_ctrls_init(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ov7251 *priv = to_ov7251(client);
	const struct ov7251_mode *mode = priv->cur_mode;
	s64 pixel_rate;
	int ret;

	v4l2_ctrl_handler_init(&priv->ctrl_handler, 10);
	
	v4l2_ctrl_new_std(&priv->ctrl_handler, &ov7251_ctrl_ops,
			  V4L2_CID_HFLIP,0,1,1,0);
	v4l2_ctrl_new_std(&priv->ctrl_handler, &ov7251_ctrl_ops,
			  V4L2_CID_VFLIP,0,1,1,0);

	v4l2_ctrl_new_std(&priv->ctrl_handler, &ov7251_ctrl_ops,
			  V4L2_CID_GAIN,
			  OV7251_DIGITAL_GAIN_MIN,
			  OV7251_DIGITAL_GAIN_MAX, 1,
			  OV7251_DIGITAL_GAIN_DEFAULT);

	v4l2_ctrl_new_std(&priv->ctrl_handler, &ov7251_ctrl_ops,
			  V4L2_CID_EXPOSURE,
			  (OV7251_DIGITAL_EXPOSURE_MIN) ,
			  (OV7251_DIGITAL_EXPOSURE_MAX), 1,
			  (OV7251_DIGITAL_EXPOSURE_DEFAULT) );
			  

	/* freq */
	v4l2_ctrl_new_int_menu(&priv->ctrl_handler, NULL, V4L2_CID_LINK_FREQ,
			       0, 0, link_freq_menu_items);
	pixel_rate = mode->vts_def * mode->hts_def * mode->max_fps;
	priv->pixel_rate = v4l2_ctrl_new_std(&priv->ctrl_handler, NULL, V4L2_CID_PIXEL_RATE,
			  0, pixel_rate, 1, pixel_rate);


	priv->subdev.ctrl_handler = &priv->ctrl_handler;
	if (priv->ctrl_handler.error) {
		dev_err(&client->dev, "Error %d adding controls\n",
			priv->ctrl_handler.error);
		ret = priv->ctrl_handler.error;
		goto error;
	}

	ret = v4l2_ctrl_handler_setup(&priv->ctrl_handler);
	if (ret < 0) {
		dev_err(&client->dev, "Error %d setting default controls\n",
			ret);
		goto error;
	}

	return 0;
error:
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
	return ret;
}

/* Return 0 if detection is successful, -ENODEV otherwise */
static int ov7251_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	//struct i2c_adapter *adapter = client->adapter;

	strlcpy(info->type, "ov7251", I2C_NAME_SIZE);

	return 0;
}

static int ov7251_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct ov7251 *priv;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_warn(&adapter->dev,
			 "I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE\n");
		return -EIO;
	}

	priv = devm_kzalloc(&client->dev, sizeof(struct ov7251), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
 	
 	priv->rom = i2c_new_dummy_device(adapter,0x10);
 	if ( priv->rom )
 	{	
		static int i=1;
		int addr,reg,data;
		dev_info(&client->dev, "InnoMaker Camera controller found!\n");

		for (addr=0; addr<sizeof(priv->rom_table); addr++)
		{
	          reg = rom_read(priv->rom, addr);
		  *((char *)(&(priv->rom_table))+addr)=(char)reg;
		  dev_dbg(&client->dev, "addr=0x%04x reg=0x%02x\n",addr,reg);
		}

		dev_info(&client->dev, "[ MAGIC  ] [ %s ]\n",
				priv->rom_table.magic);

		dev_info(&client->dev, "[ MANUF. ] [ %s ] [ MID=0x%04x ]\n",
				priv->rom_table.manuf,
				priv->rom_table.manuf_id);

		dev_info(&client->dev, "[ SENSOR ] [ %s %s ]\n",
				priv->rom_table.sen_manuf,
				priv->rom_table.sen_type);

		dev_info(&client->dev, "[ MODULE ] [ ID=0x%04x ] [ REV=0x%04x ]\n",
				priv->rom_table.mod_id,
				priv->rom_table.mod_rev);

		dev_info(&client->dev, "[ MODES  ] [ NR=0x%04x ] [ BPM=0x%04x ]\n",
				priv->rom_table.nr_modes,
				priv->rom_table.bytes_per_mode);

		addr = 200;// reset
	       	data = 2; //  powerdown sensor 
	        reg = rom_write(priv->rom, addr, data);
		
		addr = 202; // mode
	       	data = sensor_mode; // default 8-bit streaming
	        reg = rom_write(priv->rom, addr, data);
		
		
		while(1)
		{
			mdelay(120); // wait 100ms 
			
			addr = 201; // status
			reg = rom_read(priv->rom, addr);
			
			if(reg & 0x80)
			       	break;
			
			if(reg & 0x01) 	
				dev_err(&client->dev, "!!! ERROR !!! setting  Sensor MODE=%d STATUS=0x%02x i=%d\n",sensor_mode,reg,i);
			
			if(i++ >  4)
				break;
		}

		dev_info(&client->dev, "Sensor MODE=%d PowerOn STATUS=0x%02x i=%d\n",sensor_mode,reg,i);

	}
	else
	{
		dev_err(&client->dev, "NOTE !!!  External Camera controller  not found !!!\n");
		dev_info(&client->dev, "Sensor MODE=%d \n",sensor_mode);
		return -EIO;
	}

	/* 640 * 480 by default */
	priv->cur_mode = &supported_modes[sensor_mode];

	v4l2_i2c_subdev_init(&priv->subdev, client, &ov7251_subdev_ops);
	ret = ov7251_ctrls_init(&priv->subdev);
	if (ret < 0)
		return ret;
	ret = ov7251_video_probe(client);
	if (ret < 0)
		return ret;

	priv->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	priv->pad.flags = MEDIA_PAD_FL_SOURCE;

	priv->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&priv->subdev.entity, 1, &priv->pad);
	if (ret < 0)
		return ret;

	ret = v4l2_async_register_subdev(&priv->subdev);
	if (ret < 0)
		return ret;

	return ret;
}
#if LINUX_VERSION_CODE>= KERNEL_VERSION(6,1,0) 
void ov7251_remove(struct i2c_client *client)
#else
static int ov7251_remove(struct i2c_client *client)
#endif	
{
	struct ov7251 *priv = to_ov7251(client);

	if(priv->rom)
		i2c_unregister_device(priv->rom);
	v4l2_async_unregister_subdev(&priv->subdev);
	media_entity_cleanup(&priv->subdev.entity);
	v4l2_ctrl_handler_free(&priv->ctrl_handler);
#if LINUX_VERSION_CODE>= KERNEL_VERSION(6,1,0) 
    return;
#else	
	return 0;
#endif
}

static const struct i2c_device_id ov7251_id[] = {
	{"inno_mipi_ov7251", 0},
	{}
};

static const struct of_device_id ov7251_of_match[] = {
	{ .compatible = "omnivision,inno_mipi_ov7251" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ov7251_of_match);

MODULE_DEVICE_TABLE(i2c, ov7251_id);
static struct i2c_driver ov7251_i2c_driver = {
	.driver = {
		.of_match_table = of_match_ptr(ov7251_of_match),
		.name = "inno_mipi_ov7251",
	},
	.probe = ov7251_probe,
	.remove = ov7251_remove,
	.id_table = ov7251_id,
	.class		= I2C_CLASS_HWMON,
	.detect		= ov7251_detect,
	.address_list	= normal_i2c,
};

module_i2c_driver(ov7251_i2c_driver);
MODULE_VERSION("0.0.1");
MODULE_DESCRIPTION("Inno-maker - MIPI OV7251 driver for Raspberry pi");
MODULE_AUTHOR("Jack Yang, Inno-maker  <support@inno-maker.com>");
MODULE_LICENSE("GPL v2");
