#include "timeManager.h"

#include <RTI/Enums.h>
#include <RTI/RTI1516.h>
#include <RTI/RTI1516fedTime.h>

#include <iostream>
#include <string>
#include <cstring>
#include <memory>

using namespace std;


int main(int argc, char** argv)
{
    wstring federate_name(argv[1], argv[1] + strlen(argv[1]));
    wstring federation_name(argv[2], argv[2] + strlen(argv[2]));

    try
    {
        Time time(federation_name, federate_name);

        time.run();
    }
    catch(rti1516e::Exception & e)
    {
        wcout << "Error: " << e.what() << endl;

        return EXIT_FAILURE;
    }
    
    return 0;
}