#ifndef SignInData_h
#define SignInData_h
#include "Arduino.h"

class SignInData
{
public:
    SignInData();
    const char* getSSID();
    const char* getPassword();
    const char* getgScriptID();
    void init();
    const char* SSID;
    const char* Password;
    const char* gScriptID;
private:

    
};
#endif
