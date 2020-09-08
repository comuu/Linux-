
static struct usb_device_id skel_table[] = {
	{USB_DEVICE(USB_SKEL_VENDOR_ID,USB_SKEL_PRODUCT_ID)},
	{}
};
MODULE_DEVICE_TABLE(usb,skel_table);


static struct usb_driver skel_driver = {
	.owner = THIS_MODULE,
	.name = "skeleton",
	.id_table = skel_table,
	.probe = skel_probe,
	.disconnect = skel_disconnect,
}



static int __init usb_skel_init(void)
{
	int result;
	result = usb_register(&skel_driver);
	if(result)
		err("usb_register failed.Error number is %d",result);
	return result;
}

static void __exit usb_skel_exit(void)
{
	usb_deregister(&skel_driver);
}
module_init(usb_skel_init);
module_exit(usb_skel_exit);
MODULE_LICENSE("GPL");
