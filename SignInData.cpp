/*  HTTPS on ESP8266 with follow redirects, chunked encoding support
 *  Version 2.1
 *  Author: Sujay Phadke
 *  Github: @electronicsguy
 *  Copyright (C) 2017 Sujay Phadke <electronicsguy123@gmail.com>
 *  All rights reserved.
 *
 */

#include "SignInData.h"
#include "Arduino.h"

const char* _SSID = "ssid";
const char* _Password = "abc213";
const char* _gScriptID = "";

SignInData::SignInData(){
    
    SSID = _SSID;
    Password = _Password;
    gScriptID = _gScriptID;
}
void SignInData::init(){
    /*SignInData.SSID = _SSID;
    SignInData.Password = Password;
    SignInData.gScriptID = _gScriptID;*/
    
}
const char* SignInData::getSSID(){
    
    return _SSID;
}
const char* SignInData::getPassword(){
    
    return _Password;
}
const char* SignInData::getgScriptID(){
    return  _gScriptID;
}

