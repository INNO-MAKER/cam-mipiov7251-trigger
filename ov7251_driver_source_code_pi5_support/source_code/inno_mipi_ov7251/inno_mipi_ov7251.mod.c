#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x4e8e25be, "i2c_register_driver" },
	{ 0x9166fada, "strncpy" },
	{ 0x5c362e8, "i2c_unregister_device" },
	{ 0x33297427, "v4l2_async_unregister_subdev" },
	{ 0x15be55ad, "v4l2_subdev_cleanup" },
	{ 0x263f3602, "v4l2_ctrl_handler_free" },
	{ 0xa2eb131b, "__v4l2_ctrl_modify_range" },
	{ 0xbbe69956, "i2c_transfer" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x40dd8548, "_dev_warn" },
	{ 0x2cfa9196, "i2c_del_driver" },
	{ 0xbebbdd25, "_dev_info" },
	{ 0xf9a482f9, "msleep" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0xb6e6d99d, "clk_disable" },
	{ 0xb077e70a, "clk_unprepare" },
	{ 0x815588a6, "clk_enable" },
	{ 0x36a78de3, "devm_kmalloc" },
	{ 0xa60c35c2, "i2c_new_dummy_device" },
	{ 0xa8eb0c77, "v4l2_i2c_subdev_init" },
	{ 0x247cff22, "__v4l2_subdev_init_finalize" },
	{ 0x3ed097c3, "_dev_err" },
	{ 0x9aaf43cd, "v4l2_ctrl_handler_init_class" },
	{ 0x75c4b665, "v4l2_ctrl_new_std" },
	{ 0x9e781eac, "v4l2_ctrl_new_int_menu" },
	{ 0xc38b7ac0, "v4l2_ctrl_handler_setup" },
	{ 0xa3732c8f, "media_entity_pads_init" },
	{ 0x8444a114, "__v4l2_async_register_subdev" },
	{ 0x8b7bb3fc, "param_ops_int" },
	{ 0x474e54d2, "module_layout" },
};

MODULE_INFO(depends, "v4l2-async,videodev,mc");

MODULE_ALIAS("of:N*T*Covti,ov7251");
MODULE_ALIAS("of:N*T*Covti,ov7251C*");
MODULE_ALIAS("i2c:inno_mipi_ov7251");

MODULE_INFO(srcversion, "1B7A0098238EF1E02750C21");
