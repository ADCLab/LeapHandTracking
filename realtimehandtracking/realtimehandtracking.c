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
int64_t lastFrameID = 0; //The last frame received

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
 
static size_t write_data(void *data, size_t size, size_t nmemb, void *clientp)
{
    size_t realsize = size * nmemb;
    memory *mem = (memory *)clientp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if(ptr == NULL)
        return 0;  /* out of memory! */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

typedef struct {
    uint32_t airspace_state;
    uint32_t compass;
    uint32_t decision_state;
    uint32_t departure_index;
    uint32_t destination_index;
    uint32_t flight_start_time;
    double latitude;
    double longitude;
    uint32_t pre_trial;
    uint32_t reset_user_display;
    uint32_t reset_vitals_display;
    uint32_t sequence;
    uint32_t study_stage;
    uint32_t time_to_destination;
    uint32_t user_id;
    uint32_t vitals_state;

} flaskData;

flaskData parseFlaskData(char* url) {

    CURL *curl_handle;
    CURLcode res;
    memory chunk;

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://ip.jsontest.com/"); // url for testing, to be changed.
    res = curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);

    printf("%s", chunk.response);

    cJSON *json = cJSON_Parse(chunk.response)->child;
    flaskData data;
    data.airspace_state = json->valueint;
    json = json->next;
    data.compass = json->valueint;
    json = json->next;
    data.decision_state = json->valueint;
    json = json->next;
    data.departure_index = json->valueint;
    json = json->next;
    data.destination_index = json->valueint;
    json = json->next;
    data.flight_start_time = json->valueint;
    data.latitude = json->valuedouble;
    json = json->next;
    data.longitude = json->valuedouble;
    json = json->next;
    data.pre_trial = json->valueint;
    json = json->next;
    data.reset_user_display = json->valueint;
    json = json->next;
    data.reset_vitals_display = json->valueint;
    json = json->next;
    data.sequence = json->valueint;

    cJSON_Delete(json);

    return data;
}

int main(int argc, char** argv) {

    // Open log file
    if (argc < 2) {
        printf("Please enter a file name.\n");
        exit(1);
    }
    logFile = fopen(argv[1], "w");

    //Set callback function pointers
    ConnectionCallbacks.on_connection = &OnConnect;
    ConnectionCallbacks.on_device_found = &OnDevice;
    ConnectionCallbacks.on_frame = &OnFrame;
    //ConnectionCallbacks.on_image = &OnImage;
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
//End-of-Sample