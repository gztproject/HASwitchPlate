#ifndef WEB_H
#define WEB_H

class web
{
public:
    web();

private:
};

void webHandleNotFound();
void webHandleRoot();
void webHandleSaveConfig();
void webHandleResetConfig();
void webHandleResetBacklight();
void webHandleFirmware();
void webHandleEspFirmware();
void webHandleLcdUpload();
void webHandleLcdUpdateSuccess();
void webHandleLcdUpdateFailure();
void webHandleLcdDownload();
void webHandleTftFileSize();
void webHandleReboot();

#endif