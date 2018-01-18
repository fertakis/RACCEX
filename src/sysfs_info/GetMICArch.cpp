#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    char str[257] =  { 0 };
    char path[257] = { 0 };
    snprintf(path, 256, "/sys/class/mic/mic%d/family", 0);
    FILE *scif_sysfs = fopen(path, "r");
    if (scif_sysfs != NULL)
    {
        fread(str, sizeof(char), 256, scif_sysfs);
        fclose(scif_sysfs);
        std::string scif_arch(str);
        std::string::size_type pos = scif_arch.find_last_not_of('\n');
        if (pos != scif_arch.length() - 1)
        {
            if (pos == std::string::npos)
                pos = -1;
            scif_arch.erase(pos + 1);
        }

        if (strcmp(scif_arch.c_str(), "x100") == 0)
        {
		std::cout <<"Knight Crossing\n";
        }
        else if (strcmp(scif_arch.c_str(), "x200") == 0)
        {
		std::cout <<"Knights Landing\n";
        }
	else 
	{
		std::cout <<"Wrong Device Type\n";
	}
    }
    else 
	    std::cout <<"File doesn't exist!\n";
    return 0;
}

