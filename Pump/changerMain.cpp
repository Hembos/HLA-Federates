#include "changer.h"

#include <RTI/Enums.h>
#include <RTI/RTI1516.h>
#include <RTI/RTI1516fedTime.h>

#include <iostream>
#include <string>
#include <cstring>

using namespace std;


int main(int argc, char** argv)
{
    wstring federate_name(argv[1], argv[1] + strlen(argv[1]));
    wstring federation_name(argv[2], argv[2] + strlen(argv[2]));

    try
    {
        cout << "start" << endl;
        Changer* changer = new Changer(federation_name, federate_name);

        changer->run();

        cout << "endl" << endl;
        delete changer;
    }
    catch(rti1516e::Exception & e)
    {
        wcout << "Error: " << e.what() << endl;

        return EXIT_FAILURE;
    }
    catch(exception & e)
    {
        wcout << "Error: " << e.what() << endl;

        return EXIT_FAILURE;
    }
    
    return 0;
}