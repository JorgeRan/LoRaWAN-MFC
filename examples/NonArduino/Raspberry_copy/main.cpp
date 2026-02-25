// Needed for uint8_t
#include <cstdint>
#include <RadioLib.h>
#include <wiringPi.h> // Include GPIO library

#define SWITCH_PIN 0 // Define the GPIO pin for the switch

// void setupSwitch() {
//     wiringPiSetup(); // Initialize wiringPi
//     pinMode(SWITCH_PIN, INPUT); // Set the switch pin as input
// }
//
// bool buttonPressed = true;
//
// bool isButtonPressed() {
//     static unsigned long lastDebounceTime = 0;
//     unsigned long currentTime = millis();
//     const unsigned long DEBOUNCE_DELAY = 50; // 50ms debounce
//
//     if (digitalRead(SWITCH_PIN) == HIGH) {
//         if (currentTime - lastDebounceTime > DEBOUNCE_DELAY) {
//             lastDebounceTime = currentTime;
//             return true;
//         }
//     }
//     return false;
// }

#include "hal/RPi/PiHal.h"
#include <string>
#include <iostream>
#include <cstring>
#include <cmath>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <nlohmann/json.hpp>
#include <future>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


PiHal* hal = new PiHal(0);

SX1262 radio = new Module(hal, 21, 16, 18, 20);

const LoRaWANBand_t band = US915;
LoRaWANNode node(&radio, &band);

uint32_t devAddr = 0x260C5798;
uint8_t fNwkSIntKey[] = { 0x75, 0x52, 0x2B, 0x33, 0x0C, 0x26, 0xC9, 0xD9, 0x8E, 0x7B, 0x25, 0x6A, 0xA2, 0xCC, 0x01, 0x5E };
uint8_t sNwkSIntKey[] = { 0x52, 0x30, 0xE0, 0xA1, 0xB0, 0xA0, 0xAD, 0xFA, 0x24, 0x48, 0xCD, 0x42, 0xCA, 0x08, 0x1D, 0x40 };
uint8_t nwkSEncKey[] = { 0x7E, 0xD7, 0x0F, 0xEC, 0x10, 0xEE, 0xEE, 0x68, 0xA7, 0xA5, 0x9D, 0xD6, 0x32, 0x28, 0x17, 0xE9 };
uint8_t appSKey[] = { 0x65, 0xB2, 0xB1, 0xEE, 0xA4, 0x10, 0x72, 0xC2, 0x1D, 0xFC, 0x9B, 0xA9, 0xEC, 0xD1, 0x24, 0xDE };

bool deviceOn = false;

char device_1[3] = "XX";
char device_2[3] = "XX";

float flow_1 = 0.0f;
float flow_2 = 0.0f;

float mfcSetpoint_1 = 0.0f;
float mfcSetpoint_2 = 0.0f;

FILE* status_fp = nullptr;
int status_fd = -1;
std::string partial;
bool statusPublisherRunning = false;

const std::map<std::string, uint8_t> GAS_MAP = {
        {"AIR", 0x00},
        {"NITROGEN", 0x01},
        {"METHANE", 0x02},
        {"CARBON DIOXIDE", 0x03},
        {"PROPANE", 0x04},
        {"BUTANE", 0x05},
        {"ETHANE", 0x06},
        {"HYDROGEN", 0x07},
        {"CARBON MONOXIDE", 0x08},
        {"ACETYLENE", 0x09},
        {"ETHYLENE", 0x0a},
        {"PROPYLENE", 0x0b},
        {"BUTYLENE", 0x0c},
        {"NITROUS OXIDE", 0x0d},
      };

void downlinkAction(const uint8_t *uplink, size_t uplinkLen);



//--------------------------------PYTHON SCRIPTS------------------------------------



void sendSetpointToPython(float setpoint, uint32_t mfc_id, int extra_arg) {
    const char* host = "127.0.0.1";
    const int port = 8765;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("[WARNING] socket() failed, falling back to subprocess\n");
        goto fallback;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        printf("[WARNING] Could not connect to TCP %s:%d, falling back to subprocess\n", host, port);
        goto fallback;
    }

    try {
        nlohmann::json cmd;
        cmd["action"] = "setpoint";
        cmd["mfc_id"] = mfc_id;
        cmd["setpoint"] = setpoint;

        std::string s = cmd.dump() + "\n";
        ssize_t w = write(fd, s.c_str(), s.size());
        if (w < 0) {
            printf("[WARNING] Failed to write to socket, falling back to subprocess\n");
            close(fd);
            goto fallback;
        }

        char buf[256];
        ssize_t r = read(fd, buf, sizeof(buf) - 1);
        if (r > 0) {
            buf[r] = '\0';
            try {
                auto resp = nlohmann::json::parse(buf);
                bool ok = resp.value("success", false);
                std::string msg = resp.value("message", "");
                if (ok) {
                    printf("[INFO] Setpoint %.2f sent to socket handler (MFC ID %u): %s\n", setpoint, (unsigned)mfc_id, msg.c_str());
                    
                    uint8_t ack_uplink[2];
                    ack_uplink[0] = 0x11;  
                    ack_uplink[1] = (uint8_t)mfc_id;
                    downlinkAction(ack_uplink, sizeof(ack_uplink));

                } else {
                    printf("[WARNING] Socket handler returned failure: %s\n", msg.c_str());
                    close(fd);
                    goto fallback;
                }
            } catch (...) {
                close(fd);
                goto fallback;
            }
        } else {
            close(fd);
            goto fallback;
        }

        close(fd);
        return;
    } catch (...) {
        close(fd);
        printf("[WARNING] Exception while using socket, falling back to subprocess\n");
        goto fallback;
    }

fallback:
    {
        std::stringstream cmd;
        cmd << "bash -c \"cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/mass-flow-controller && source /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/mass-flow-controller/.venv/bin/activate >/dev/null 2>&1 && python3 mfc_setpoint_controller.py "
             << setpoint << " " << mfc_id << " " << extra_arg << "\"";
        int result = system(cmd.str().c_str());
        if (result != 0) {
            printf("[WARNING] Failed to send setpoint to Python script (return code: %d)\n", result);
        } else {
            printf("[INFO] Setpoint %.2f sent to Python script with MFC ID %u\n", setpoint, (unsigned)mfc_id);
            uint8_t ack_uplink[2];
            ack_uplink[0] = 0x11;  
            ack_uplink[1] = (uint8_t)mfc_id;
            downlinkAction(ack_uplink, sizeof(ack_uplink));
        }
    }
}

void sendRefreshToPython() {
    const char* host = "127.0.0.1";
    const int port = 8765;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("[WARNING] socket() failed for refresh\n");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        printf("[WARNING] Could not connect to TCP %s:%d for refresh\n", host, port);
        return;
    }

    try {
        nlohmann::json cmd;
        cmd["action"] = "refresh";

        std::string s = cmd.dump() + "\n";
        ssize_t w = write(fd, s.c_str(), s.size());
        if (w < 0) {
            printf("[WARNING] Failed to write refresh to socket\n");
            close(fd);
            return;
        }

        char buf[256];
        ssize_t r = read(fd, buf, sizeof(buf) - 1);
        if (r > 0) {
            buf[r] = '\0';
            try {
                auto resp = nlohmann::json::parse(buf);
                bool ok = resp.value("success", false);
                std::string msg = resp.value("message", "");
                if (ok) {
                    printf("[INFO] Refresh sent to socket handler: %s\n", msg.c_str());
                } else {
                    printf("[WARNING] Socket handler returned failure for refresh: %s\n", msg.c_str());
                }
            } catch (...) {
                printf("[WARNING] Exception parsing refresh response\n");
            }
        } else {
            printf("[WARNING] No response for refresh\n");
        }

        close(fd);
    } catch (...) {
        close(fd);
        printf("[WARNING] Exception while sending refresh\n");
    }
}

FILE* startStatusPublisher() {
    std::stringstream cmd;
    cmd << "bash -c \"cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/mass-flow-controller && source /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/mass-flow-controller/.venv/bin/activate >/dev/null 2>&1 && python3 mfc_status_publisher.py\"";
    FILE* fp = popen(cmd.str().c_str(), "r");
    if(!fp) {
        printf("[ERROR] Failed to start status publisher\n");
        return nullptr;
    }
    int fd = fileno(fp);
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return fp;
}

//------------------------DOWNLINK------------------------------


// Send gas selection to Python server for calibration tracking
void sendGasToPython(uint8_t mfc_id, uint8_t gas_code) {
    const char* host = "127.0.0.1";
    const int port = 8765;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        printf("[WARNING] socket() failed for gas command\n");
        return;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        printf("[WARNING] Could not connect to TCP %s:%d for gas command\n", host, port);
        return;
    }
    try {
        nlohmann::json cmd;
        cmd["action"] = "gas";
        cmd["mfc_id"] = mfc_id;
        cmd["gas_cmd"] = gas_code;
        std::string s = cmd.dump() + "\n";
        ssize_t w = write(fd, s.c_str(), s.size());
        if (w < 0) {
            printf("[WARNING] Failed to write gas command to socket\n");
            close(fd);
            return;
        }
        char buf[256];
        ssize_t r = read(fd, buf, sizeof(buf) - 1);
        if (r > 0) {
            buf[r] = '\0';
            printf("[INFO] Gas command response: %s\n", buf);
        }
        close(fd);
    } catch (...) {
        close(fd);
        printf("[WARNING] Exception while sending gas command\n");
    }
}

void downlinkAction(const uint8_t *uplink, size_t uplinkLen) {
    uint8_t downlink[32];
    size_t dlLen = sizeof(downlink);
    int sendResult = node.sendReceive(uplink, uplinkLen, 1, downlink, &dlLen, false);
    if(sendResult >= 0) {
        printf("[Status uplink] sent (result=%d)\n", sendResult);
        printf("success! (downlink: %d)\n", sendResult);
            printf("[Downlink] %zu bytes: ", dlLen);
            for(size_t i = 0; i < dlLen; i++) {
                printf("%02X ", downlink[i]);
            }
            printf("\n");

            uint8_t cmd = downlink[0];

            if(cmd == 0x01 && dlLen <= 10){
                deviceOn = true;
                printf("Command: ON\n");
            } else if(cmd == 0x00 && dlLen <= 10){
                deviceOn = false;
                printf("Command: OFF\n");
            } else if(cmd == 0x10 && dlLen >= 6 && dlLen <= 15){
                uint32_t raw = ((uint32_t)downlink[2] << 24) | ((uint32_t)downlink[3] << 16) | ((uint32_t)downlink[4] << 8) | ((uint32_t)downlink[5]);
                float sp;
                memcpy(&sp, &raw, 4);
                if(downlink[1] == 0) {
                    mfcSetpoint_1 = sp;
                    printf("Command: SETPOINT for MFC 0 (BL) = %f\n", mfcSetpoint_1);
                    sendSetpointToPython(mfcSetpoint_1, downlink[1], 0);
                } else if(downlink[1] == 1) {
                    mfcSetpoint_2 = sp;
                    printf("Command: SETPOINT for MFC 1 (BF) = %f\n", mfcSetpoint_2);
                    sendSetpointToPython(mfcSetpoint_2, downlink[1], 0);
                }

            } else if (cmd == 0x11 && dlLen <= 10) {
                printf("Received command to refresh data\n");
                sendRefreshToPython();

            } else if (cmd == 0x21 && dlLen <= 10) {
                printf("Command: GAS for calibration\n");
                if (dlLen < 3) {
                    printf("Invalid GAS command length: %zu\n", dlLen);
                    return;
                }
                uint8_t mfc_1 = downlink[1];
                uint8_t gasCode_1 = downlink[2];
                bool hasMfc2 = dlLen >= 5;
                uint8_t mfc_2 = hasMfc2 ? downlink[3] : 0xFF;
                uint8_t gasCode_2 = hasMfc2 ? downlink[4] : 0xFF;
                auto getGasName = [](uint8_t code) -> std::string {
                    for (const auto& pair : GAS_MAP) {
                        if (pair.second == code) {
                            return pair.first;
                        }
                    }
                    return "UNKNOWN";
                };
                printf("MFC1=%u Gas=0x%02X (%s)\n",
                    mfc_1,
                    gasCode_1,
                    getGasName(gasCode_1).c_str());
                sendGasToPython(mfc_1, gasCode_1);
                if (hasMfc2) {
                    printf("MFC2=%u Gas=0x%02X (%s)\n",
                        mfc_2,
                        gasCode_2,
                        getGasName(gasCode_2).c_str());
                    sendGasToPython(mfc_2, gasCode_2);
                }
            }

        } else if(sendResult == 0) {
            printf("Succes! (no downlink)\n");
        } else if(sendResult == -999) {
            printf("sendReceive timed out; attempting to continue loop\n");

            uint8_t uplink[3];
            uplink[0] = 0x1F;     
            uplink[1] = 0x03;
            uplink[2] = 0x04;

            downlinkAction(uplink, sizeof(uplink));
        } else {
            printf("failed, code %d\n", sendResult);

            uint8_t uplink[3];
            uplink[0] = 0x1F;     
            uplink[1] = 0x03;
            uplink[2] = 0x05;

            downlinkAction(uplink, sizeof(uplink));
    		}
        }
    

    //-----------------------UPLINKS-----------------------------

void errorUplink(uint8_t errorSource, uint8_t code) {
    uint8_t uplink[3];
    uplink[0] = 0x1F;     
    uplink[1] = errorSource;
    uplink[2] = code;

    downlinkAction(uplink, sizeof(uplink));
}

void hearbeatUplink() {
    uint8_t uplink[2];
    uplink[0] = 0x30;     
    uplink[1] = 0x00;   

   downlinkAction(uplink, sizeof(uplink));
}

void sendStatus(float setpoint, float flow, uint8_t mfc_id, char device[2]) {
    uint8_t uplink[12];

    uplink[0] = 0x20;     
    uplink[1] = mfc_id;  

    uint32_t raw;
    
    memcpy(&raw, &setpoint, sizeof(float));
    uplink[2] = (raw >> 24) & 0xFF;
    uplink[3] = (raw >> 16) & 0xFF;
    uplink[4] = (raw >> 8)  & 0xFF;
    uplink[5] =  raw        & 0xFF;

    memcpy(&raw, &flow, sizeof(float));
    uplink[6] = (raw >> 24) & 0xFF;
    uplink[7] = (raw >> 16) & 0xFF;
    uplink[8] = (raw >> 8)  & 0xFF;
    uplink[9] =  raw        & 0xFF;

    uplink[10] = device[0];
    uplink[11] = device[1];

    printf("[INFO] Sending status uplink for MFC %u: setpoint=%.2f, flow=%.2f\n", (unsigned)mfc_id, setpoint, flow);
    downlinkAction(uplink, sizeof(uplink));
}


//-------------------------MAIN--------------------------------------

int main(int argc, char** argv) {
    // Clean up all GPIO pins to avoid "GPIO not allocated" errors
    printf("[INFO] Cleaning up GPIO pins...\n");
    system("gpio unexportall 2>/dev/null");
    usleep(500000); // Wait 500ms for GPIO cleanup
    
    // setupSwitch(); // DISABLED: Switch not available
    printf("[INFO] Starting uplink/downlink process...\n");
    // printf("[INFO] Press the switch again to stop the program...\n");
    printf("[SX1262] Initializing radio ... ");
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        printf("failed, code %d\n", state);
        // errorUplink(0x03, 0x06);
        return(1);
    }
    printf("success!\n");
    
    
//----------------------LoRaWAN SETUP-----------------------------------
    printf("[LoRaWAN] Setting up ABP ... ");
    state = node.beginABP(devAddr, fNwkSIntKey, sNwkSIntKey, nwkSEncKey, appSKey);
    if (state != RADIOLIB_ERR_NONE) {
        printf("failed, code %d\n", state);
        // errorUplink(0x04, 0x07);
        return(1);
    }

    state = node.activateABP();
    if (state < 0 && state != RADIOLIB_LORAWAN_SESSION_RESTORED && state != RADIOLIB_LORAWAN_NEW_SESSION) {
        printf("ABP activation failed, code %d\n", state);
        // errorUplink(0x04, 0x07);
        return(1);
    }
    printf("success!\n");

    printf("Ready!\n");

    status_fp = startStatusPublisher();
    status_fd = status_fp ? fileno(status_fp) : -1;
    partial = "";

    int count = 0;
    int mfc_id = 0;
    char device[3] = "XX";

    for(;;) {
        // DISABLED: Switch button check
        // if (isButtonPressed()) {
        //     printf("[INFO] Stop button pressed, shutting down...\n");
        //     if (status_fp) {
        //         pclose(status_fp);
        //     }
        //     return(0);
        // }

//-------------------PYTHON SCRIPTS POLLIN------------------------------
        
        if(status_fd >= 0) {
            struct pollfd pfd;
            pfd.fd = status_fd;
            pfd.events = POLLIN;
            int rv = poll(&pfd, 1, 0);
            if(rv > 0 && (pfd.revents & POLLIN)) {
                char buf[512];
                ssize_t n = read(status_fd, buf, sizeof(buf)-1);
                if(n > 0) {
                    buf[n] = '\0';
                    partial += buf;
                    size_t pos;
                    while((pos = partial.find('\n')) != std::string::npos) {
                        std::string line = partial.substr(0, pos);

                        if(!line.empty() && line.back() == '\r') line.pop_back();
                        partial.erase(0, pos+1);
                       
                        if (line.rfind("STATUS:", 0) == 0) {
                            char device[3] = "XX";
                            int mfc_id = -1;
                            float flow = 0.0f;
                            float setpoint = 0.0f;
                            char gas[16] = "XXXXXXXXXXXXXXX";
                            char gasName[32] = "UNKNOWN";
                            int gasCode = -1;

                            int parseResult = sscanf(line.c_str(), "STATUS:%c%c:%d:%f:%f:%s", &device[0], &device[1], &mfc_id, &flow, &setpoint, gas);
                            
                            if (parseResult != 6) {
                                parseResult = sscanf(line.c_str(), "STATUS:%d:%f", &mfc_id, &flow);
                            }

                            if (parseResult == 6) {
                                char* endPtr = nullptr;
                                long parsed = strtol(gas, &endPtr, 0);
                                if (endPtr != gas && *endPtr == '\0' && parsed >= 0 && parsed <= 255) {
                                    gasCode = static_cast<int>(parsed);
                                    for (const auto& pair : GAS_MAP) {
                                        if (pair.second == static_cast<uint8_t>(gasCode)) {
                                            snprintf(gasName, sizeof(gasName), "%s", pair.first.c_str());
                                            break;
                                        }
                                    }
                                } else {
                                    snprintf(gasName, sizeof(gasName), "%s", gas);
                                }
                            }
                            
                            if (parseResult >= 2 && mfc_id >= 0 && mfc_id <= 1) {
                                if (parseResult == 6 && gasCode >= 0) {
                                    printf("[INFO] Status  Device=%c%c MFC%d - Flow=%.4f, Setpoint=%.4f, Gas=%s (0x%02X)\n", device[0], device[1], mfc_id, flow, setpoint, gasName, gasCode);
                                } else if (parseResult == 6) {
                                    printf("[INFO] Status  Device=%c%c MFC%d - Flow=%.4f, Setpoint=%.4f, Gas=%s\n", device[0], device[1], mfc_id, flow, setpoint, gasName);
                                } else {
                                    printf("[INFO] Status  Device=%c%c MFC%d - Flow=%.4f, Setpoint=%.4f\n", device[0], device[1], mfc_id, flow, setpoint);
                                }
                                
                                if (mfc_id == 0) {
                                    flow_1 = flow;
                                    if (parseResult == 6) {
                                        mfcSetpoint_1 = setpoint;
                                        device_1[0] = device[0];
                                        device_1[1] = device[1];
                                    }
                                    sendStatus(setpoint, flow, mfc_id, device_1);
                                } else {
                                    flow_2 = flow;
                                    if (parseResult == 6) {
                                        mfcSetpoint_2 = setpoint;
                                        device_2[0] = device[0];
                                        device_2[1] = device[1];
                                    }
                                
                                    sendStatus(setpoint, flow, mfc_id, device_2);
                                }
                            } else {
                                printf("[WARNING] Failed to parse status: %s (result=%d, id=%d)\n", line.c_str(), parseResult, mfc_id);
                                // errorUplink(0x01, 0x03);
                            }

                        } else if(line.rfind("ERROR:",0) == 0) {
                            printf("[Status publisher] %s\n", line.c_str());
                            //errorUplink(0x01, 0x02);
                            
                        } else if(line.rfind("INFO:",0) == 0) {
                            //errorUplink(0x01, 0x02);
                            printf("[Status publisher] %s\n", line.c_str());
                            
                        } 
                    }
                }
            }
        }
        if(!statusPublisherRunning) {
            hearbeatUplink();

        }
        

        // printf("BL MFC Setpoint: %.2F\n", mfcSetpoint_1);
        // printf("BF MFC Setpoint: %.2F\n", mfcSetpoint_2);

        count++;
        hal->delay(5000);
    }

    return(0);
}
