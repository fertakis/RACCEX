#include <linux/module.h> 
#include <linux/init.h> 

static int __init mymodule_init (void) 
{ 
	pr_debug("Module initialized successfully \n"); 
	return 0; 
} 

static void __exit mymodule_exit (void) 
{ 
	pr_debug ("Module un initialized successfully \n"); 
} 

module_init(mymodule_init); 
module_exit(mymodule_exit); 
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("pradheepsh");

