/* Copyright (C) 2012-2017 Ultraleap Limited. All rights reserved.
 *
 * Use of this code is subject to the terms of the Ultraleap SDK agreement
 * available at https://central.leapmotion.com/agreements/SdkAgreement unless
 * Ultraleap has signed a separate license agreement with you or your
 * organisation.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <Windows.h>
#include <stdint.h>
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);
    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;
    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;
    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <curl/curl.h>
#include <cJSON.h>

#include "LeapC.h"
#include "ExampleConnection.h"

FILE *logFile;

 /** Callback for when the connection opens. */
static void OnConnect() {
    printf("Connected.\n");
}
/** Callback for when a device is found. */
static void OnDevice(const LEAP_DEVICE_INFO* props) {
    printf("Found device %s.\n", props->serial);
}
/** Callback for when a frame of tracking data is available. */
static void OnFrame(const LEAP_TRACKING_EVENT* frame) {
    if (frame->info.frame_id % 60 == 0)
        printf("Frame %lli with %i hands.\n", (long long int)frame->info.frame_id, frame->nHands);

    for (uint32_t h = 0; h < frame->nHands; h++) {
        LEAP_HAND* hand = &frame->pHands[h];
        struct timeval ts;
        gettimeofday(&ts, NULL); // return value can be ignored

        fprintf(logFile, "Timestamp : %ld sec %ld usec\n", ts.tv_sec, ts.tv_usec);
        
        fprintf(logFile, "Hand id %i: %s\n",
            hand->id,
            (hand->type == eLeapHandType_Left ? "left" : "right"));

        for(uint32_t fing = 0; fing < 5; fing++) {
            fprintf(logFile, "Finger id:%i",hand->digits[fing].finger_id);

            for(uint32_t fbone = 0; fbone < 4; fbone++){
                fprintf(logFile, "(%f,%f,%f,%f,%f,%f)",
                hand->digits[fing].bones[fbone].prev_joint.x,
                hand->digits[fing].bones[fbone].prev_joint.y,
                hand->digits[fing].bones[fbone].prev_joint.z,
                hand->digits[fing].bones[fbone].next_joint.x,
                hand->digits[fing].bones[fbone].next_joint.y,
                hand->digits[fing].bones[fbone].next_joint.z);
            }
            fprintf(logFile, "\n");
        }
        
        fprintf(logFile, "Palm: %f, %f, %f\n",
            hand->palm.position.x,
            hand->palm.position.y,
            hand->palm.position.z);

        fprintf(logFile, "Arm: %f, %f, %f, %f, %f, %f\n",
            hand->arm.prev_joint.x,
            hand->arm.prev_joint.y,
            hand->arm.prev_joint.z,
            hand->arm.next_joint.x,
            hand->arm.next_joint.y,
            hand->arm.next_joint.z);
        
    }
}


typedef struct {
  char *response;
  size_t size;

} memory;

static size_t write_data(void *data, size_t size, size_t nmemb, void *clientp) {

    size_t realsize = size * nmemb;
    memory *mem = (memory *)clientp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if(ptr == NULL) {
        return 0;  /* out of memory! */
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

typedef struct {
    char userId[100];
    uint32_t sequence;
    uint32_t studyStage;
    char destinationIndex[100];
    char departureIndex[100];
    uint32_t decisionState;
    uint32_t vitalsState;
    uint32_t airspaceState;
    uint32_t flightStartTime;
    uint32_t resetUserDisplay;
    uint32_t resetVitalsDisplay;
    uint32_t timeToDestination;
    uint32_t preTrial;
    double latitude;
    double longitude;
    uint32_t compass;

} flaskData;

int getFlaskData(flaskData* data) {

    // Get raw data from api
    CURL *curl_handle;
    CURLcode res;
    memory chunk = {0};

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://127.0.0.0:8080/var");
    res = curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);

    if (chunk.response == NULL) {
        return 1;
    }

    // Parse json into struct
    cJSON *json = cJSON_Parse(chunk.response);

    strcpy(data->userId, cJSON_GetObjectItem(json, "user-id")->valuestring);
    //data->sequence = cJSON_GetObjectItem(json, "sequence")->valueint; 
    data->studyStage = cJSON_GetObjectItem(json, "study-stage")->valueint; 
    //strcpy(data->destinationIndex, cJSON_GetObjectItem(json, "destination-index")->valuestring);
    //strcpy(data->departureIndex, cJSON_GetObjectItem(json, "departure-index")->valuestring);
    //data->decisionState = cJSON_GetObjectItem(json, "decision-state")->valueint; 
    //data->vitalsState = cJSON_GetObjectItem(json, "vitals-state")->valueint; 
    //data->airspaceState = cJSON_GetObjectItem(json, "airspace-state")->valueint; 
    //data->flightStartTime = cJSON_GetObjectItem(json, "flight-start-time")->valueint; 
    //data->resetUserDisplay = cJSON_GetObjectItem(json, "reset-user-display")->valueint; 
    //data->timeToDestination = cJSON_GetObjectItem(json, "time-to-destination")->valueint; 
    //data->preTrial = cJSON_GetObjectItem(json, "pre-trial")->valueint; 
    //data->latitude = cJSON_GetObjectItem(json, "latitude")->valuedouble; 
    //data->longitude = cJSON_GetObjectItem(json, "longitude")->valuedouble; 
    //data->compass = cJSON_GetObjectItem(json, "compass")->valueint; 

    free(chunk.response);
    cJSON_Delete(json);

    return 0;
}

typedef struct {
    float a;
    float b;
    float c;
    float d;
} Plane; // ax + by + cz + d = 0

typedef struct {
    float x;
    float y;
    float z;
} Vector;

Plane calculatePlane(Vector p1, Vector p2, Vector p3) {

    Vector u;
    u.x = p1.x - p2.x;
    u.y = p1.y - p2.y;
    u.z = p1.z - p2.z;

    Vector v;
    v.x = p1.x - p3.x;
    v.y = p1.y - p3.y;
    v.z = p1.z - p3.z;

    Vector n;
    n.x = u.y * v.z - u.z * v.y;
    n.y = u.z * v.x - u.x * v.z;
    n.z = u.x * v.y - u.y * v.x;

    Plane plane;
    plane.a = n.x;
    plane.b = n.y;
    plane.c = n.z;
    plane.d = n.x * -p1.x + n.y * -p1.y + n.z * -p1.z;

    return plane;
}
float distanceFromPlane(Plane plane, Vector p) {
    return fabs(plane.a * p.x + plane.b * p.y + plane.c * p.z + plane.d) / sqrt(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
}

int main(int argc, char** argv) {

    // Get the log file
    flaskData data;
    while (getFlaskData(&data) == 1) { sleep(0.01); }

    char filename[200] = "";
    sprintf(filename, "%s%d", data.userId, data.studyStage);
    strcat(filename, ".txt");
    logFile = fopen(filename, "w");

    //Set callback function pointers
    ConnectionCallbacks.on_connection = &OnConnect;
    ConnectionCallbacks.on_device_found = &OnDevice;
    ConnectionCallbacks.on_frame = &OnFrame;
    LEAP_CONNECTION* connection = OpenConnection();
    LeapSetPolicyFlags(*connection, eLeapPolicyFlag_Images, 0);
    printf("Press Enter to exit program.\n");
    getchar();

    // Cleanup
    fclose(logFile);
    CloseConnection();
    DestroyConnection();
    return 0;
}
