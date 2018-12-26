/*
WIFI 
  1 wlan_salt 
  2 iotpolimi
  3 vodafone 5g
  4 casa *
*/
/*
BROKER
1 aws old
2 cluster
3 aws uc15 *
*/
#define WIFI 4
#define BROKER 3 
#define MQTT_TOPIC_STATUS "smart_campus/smartbin/home/status"
#define MQTT_TOPIC_CTRL "smart_campus/smartbin/home/read"
#define PRECISION_SAMPLES 16
#define STD_SENSIBILITY 10
#define WRONG_LOOPS 20

/*********** TIME CONFIG **************/
#define START_TIME 0
#define TOTAL_TIME 1 //seconds

/*********** END TIME  **************/

/*********** PIN CONFIG  **************/
#define D4    4 //CLK - Green
#define D5    5 //DAT - Blue
/*********** END PIN  **************/


/*********** WIFI CONFIG ***********/
#if WIFI == 1
  #define SSID_WIFI "wlan_saltuaria"
  #define PASS_WIFI "antlabpolitecnicomilano"
#endif

#if WIFI == 2
  #define SSID_WIFI "IoTPolimi"
  #define PASS_WIFI "ZpvYs=gT-p3DK3wb"
#endif

#if WIFI == 3
  #define SSID_WIFI "HUAWEI-5GCPE-D858"
  #define PASS_WIFI "Vodafone5G"
#endif

#if WIFI == 4
  #define SSID_WIFI "Vodafone-Salamandra"
  #define PASS_WIFI "123456789123456789"
#endif
/*********** END WIFI ***********/

/*********** BROKER CONFIG ***********/
#if BROKER == 1
  #define MQTT_BROKER "ec2-35-166-12-244.us-west-2.compute.amazonaws.com"
#endif

#if BROKER == 2
  #define MQTT_BROKER "10.79.1.176"
#endif

#if BROKER == 3
  #define MQTT_BROKER "34.222.27.8"
#endif
/*********** END BROKER ***********/


//Bin fixed data
#define STABILISING_TIME 3000
#define OFFSET 135292655
#define SCALE_FACTOR -206.53
