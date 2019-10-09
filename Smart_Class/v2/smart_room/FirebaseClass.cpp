#include <FirebaseArduino.h>
#include <string.h>

class FirebaseClass{
  private:
   char* host;
   char* auth;
   int https_port;
   String google_script_id;

   public:
    FirebaseClass(const char *host, const char *auth, int port, String gasId){
      this->host = strdup(host);
      this->auth = strdup(auth);
      this->https_port = port;
      this->google_script_id = gasId;
    }

    void setupF(){
      Firebase.begin(this->host, this->auth);
    }

    void reconnect(){
      
    }
};
