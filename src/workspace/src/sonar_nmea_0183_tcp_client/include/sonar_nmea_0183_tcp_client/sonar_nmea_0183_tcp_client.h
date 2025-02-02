#ifndef sonar_nmea_0183_tcp_client
#define sonar_nmea_0183_tcp_client


#include <iostream>
#include <exception>

//sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//ROS
#include "ros/ros.h"
#include "geometry_msgs/PointStamped.h"
#include "sensor_msgs/NavSatFix.h"


typedef struct{
    char talkerId[2];
    double utcTime;
    
    double latitude; 
    char northOrSouth;
    
    double longitude; 
    char eastOrWest;
    
    int quality;
    
    int nbSatellites;
    
    double hdop;
    
    double antennaAltitude;
    
    double geoidalSeparation;
    
    int dgpsAge;
    int dgpsStationId;    
    
    unsigned int checksum;
} ggaData;

typedef struct{
    char talkerId[2];
    double  depthFeet;
    double  depthMeters;
    double  depthFathoms;
    unsigned int checksum;    
}dbtData;




bool extractGGA(std::string & s,ggaData & data){   
    
    int c = sscanf(s.c_str(),"$%2sGGA,%lf,%lf,%1s,%lf,%1s,%d,%d,%lf,%lf,M,%lf,M,%d,%d*%2x",&data.talkerId,&data.utcTime,&data.latitude,&data.northOrSouth,&data.longitude,&data.eastOrWest,&data.quality,&data.nbSatellites,&data.hdop,&data.antennaAltitude,&data.geoidalSeparation,&data.dgpsAge,&data.dgpsStationId);
       
    if(c >= 8){
        int longDegrees =( (double)data.longitude / 100);
        double longMinutes = (data.longitude - (longDegrees*100)) / (double)60;
        double sign = (data.eastOrWest=='E')?1:-1;
        
        data.longitude = sign* (longDegrees + longMinutes );
        
        int latDegrees = (double)(data.latitude / 100);
        double latMinutes = (data.latitude - (latDegrees*100)) / (double)60;
        sign = (data.northOrSouth=='N')?1:-1;        

        data.latitude  = sign * (latDegrees+latMinutes);

        return true;
    }
    
    return false;
}


class Sonar{

	private:
		ros::NodeHandle node;
		ros::Publisher sonarTopic;
                ros::Publisher gnssTopic;

		uint32_t depthSequenceNumber;
                uint32_t gpsSequenceNumber;
                
		std::string serverAddress;
		std::string serverPort;

		bool useDepth = true;
		bool usePosition = false;
		bool useAttitude = false;


	public:
		Sonar(char * serverAddress, char * serverPort, bool useDepth,bool usePosition,bool useAttitude) : serverAddress(serverAddress),serverPort(serverPort),useDepth(useDepth),usePosition(usePosition),useAttitude(useAttitude){

		}

		void run(){

			sonarTopic = node.advertise<geometry_msgs::PointStamped>("depth", 1000);
                        gnssTopic  = node.advertise<sensor_msgs::NavSatFix>("fix", 1000);

			ros::Rate retry_rate(1);

		        while(ros::ok()){
				int s = -1;

				try{
					std::cout << "Connecting to " << serverAddress << ":" << serverPort << "..." << std::endl;

					struct addrinfo sa,*res;

					memset(&sa,0,sizeof(sa));

					getaddrinfo(serverAddress.c_str(),serverPort.c_str(),&sa,&res);

                                        if((s = socket(res->ai_family,res->ai_socktype,res->ai_protocol))==-1){
                                                perror("socket");
                                                throw std::runtime_error("socket");
                                        }


					if(connect(s,res->ai_addr,res->ai_addrlen) == -1){
						perror("connect");
						throw std::runtime_error("connect");
					}


					//read char by char like a dumbass
					char ch;
					std::string line;

					//FIXME: Holy wasted-syscalls Batman, that's inefficient!
					while(read(s,&ch,1)==1){

						if(ros::isShuttingDown()){
							close(s);
						}

						if(ch == '\n'){
                                                    dbtData dbt;    
                                                    ggaData gga;

                                                    //parse DBT strings such as $SDDBT,30.9,f,9.4,M,5.1,F*35
                                                    if(sscanf(line.c_str(),"$%2sDBT,%lf,f,%lf,M,%lf,F*%2x",&dbt.talkerId,&dbt.depthFeet,&dbt.depthMeters,&dbt.depthFathoms,&dbt.checksum) == 5){

                                                        //TODO: checksum
                                                        //process depth

                                                        geometry_msgs::PointStamped msg;

                        			        msg.header.seq=++depthSequenceNumber;
                                                        msg.header.stamp=ros::Time::now();

                                                        msg.point.z = dbt.depthMeters;

                                                        sonarTopic.publish(msg);
                                                    }
                                                        
                                                    //parse GGA position strings
                                                    else if(extractGGA(line,gga)){
                                                        sensor_msgs::NavSatFix msg;
                                                            
                        			        msg.header.seq=++gpsSequenceNumber;
                                                        msg.header.stamp=ros::Time::now();
                                                        
                                                        switch(gga.quality){
                                                            //No fix
                                                            case 0:
                                                                msg.status.status=-1;
                                                                break;
                                                            
                                                            //GPS Fix
                                                            case 1:
                                                                msg.status.status=0;
                                                                break;
                                                                
                                                            //DGPS
                                                            case 2:
                                                                msg.status.status=2;
                                                        }
                                                        
                                                        msg.latitude  = gga.latitude;
                                                        msg.longitude = gga.longitude;
                                                        msg.altitude  = gga.antennaAltitude;
                                                        
                                                        msg.position_covariance_type= 0;

                                                            
                                                        gnssTopic.publish(msg);
                                                    }
							
                                                    //TODO: parse attitude strings
                                                    line = "";
						}
						else{
							line.append(1,ch);
						}
					}

					close(s);
					s = -1;
				}
				catch(std::exception& e){
					if(s != -1) close(s);

					s = -1;
					retry_rate.sleep();
				}
        		}
		}

};



#endif
