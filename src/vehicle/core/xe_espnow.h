#ifndef XE_ESPNOW_H
#define XE_ESPNOW_H

void initESPNow();
void sendTelemetry();
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len);

extern int latestCmd_speed;
extern int latestCmd_direction;
extern int latestCmd_lift;
extern bool latestCmd_stop;
extern unsigned long lastCmdTime;
extern float pitchEMA, rollEMA;
extern int curA, curB, curC, curD, curE;
extern bool pitchActive, rollActive;

#endif
