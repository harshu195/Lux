#include "BGTSpawner.h"
#include <string>

#define DEBUG(x) do { if(true){ std::cout <<"[" << __TIME__ << " : " << __FILE__ << " : "<< __LINE__ << "]" << x << std::endl; } } while (0) 
int BGTSpawner::counter = 0;

bool spawnNewBgt(int bgtID) {
    std::cout << sizeof(mongo::BSONObj) << std::endl;
    //connect to the database
    DEBUG("Connecting to Database...");
    mongo::DBClientConnection c;
    c.connect("localhost");
    DEBUG("Connected to Database.");    

    string IP = "54.209.204.41";
    std::cout << "HELP! BGTSpawner : 1" << std::endl;
    
    DEBUG("BSON Obj being created...");
    //Create a BSON object
    BSONObj object = BSON(GENOID<<"IP"<<IP<<"PORT"<<0 << "BGT_ID" << bgtID);
    DEBUG("BSON Obj created.");
	
    DEBUG("Creating Doc...");
    //Create a new document
    c.insert(DATABASE_NAME,object);
    BSONObj bgt_doc = c.findOne(DATABASE_NAME,QUERY("BGT_ID"<<bgtID));
    DEBUG("Doc created.");    

    s_bgt_params_in *bgt_params_in = new s_bgt_params_in;
    
    // get the newly created _id
    bgt_params_in->bgtID = bgt_doc["_id"].toString(false);
   
    std::cout << "BGT id: " << bgt_params_in->bgtID << std::endl; 
    //string error = c.getLastError();
   
    pthread_t BGT_ID;
    pthread_t SNR_ID;
    pthread_t SUT_ID;
    pthread_t DBW_ID;

    DEBUG("Beginning pipe work...");
    const char *bgt_sut_pipeLocation ="./lux_pipe0";// xx.c_str(); // bgt->sut
    const char *sut_db_pipeLocation ="./lux_pipe1";// yy.c_str(); //"./lux_pipe1"; // sut->dbwriter
    const char *hmbl_snr_pipeLocation = "./lux_pipe2";//zz.c_str(); //"./lux_pipe2"; // HMBL->SNR
    

    if(mkfifo(bgt_sut_pipeLocation, 0666) < 0){
	cout<<"Error creating bgt_sut_pipe"<<std::endl;
    }
    cout<<"bgt_sut_pipe is created"<<std::endl;
    bgt_params_in->pipe_w = bgt_sut_pipeLocation;
  
    
    bgt_params_in->pipe_hmbl = hmbl_snr_pipeLocation;

    std::cout << bgt_params_in->pipe_w << std::endl; 
    //Spawn a Battleground thread
    pthread_create(&BGT_ID,NULL,BattleGround::spawn, bgt_params_in);
    
    s_sut_params_in *sut_params_in = new s_sut_params_in;
    sut_params_in->pipe_r = bgt_sut_pipeLocation;
    
    if(mkfifo(sut_db_pipeLocation, 0666) < 0){  
	cout<<"Error creating sut_db_pipe"<<std::endl;
    }

    cout<<"sut_db_pipe is created"<<std::endl;

    sut_params_in->pipe_w = sut_db_pipeLocation;
    
   DEBUG("Finished pipe work.");

   DEBUG("Beginning thread spawn...");
   //Spawn a SUT thread
   pthread_create(&SUT_ID,NULL,SendUpdate::spawn, sut_params_in);
    
    s_dbWriter_params_in *dbWriter_params_in = new s_dbWriter_params_in;
    dbWriter_params_in->pipe_r = sut_db_pipeLocation;
    
    //Spawn  DBWriter thread
    // pthread_create(&DBW_ID,NULL,DBWriter::spawn, dbWriter_params_in);
    

    if(mkfifo(hmbl_snr_pipeLocation,0666) < 0){
	cout<<"Error creating hmbl_snr_pipe"<<std::endl;
     }

    cout<<"hmbl_snr_pipe created"<<std::endl;
    
    s_snr_params_in *snr_params_in = new s_snr_params_in;
    snr_params_in->pipe_r = hmbl_snr_pipeLocation;    

    //Spawn snr thread
   pthread_create(&SNR_ID,NULL,SendNewRelevant::spawn, snr_params_in);    

    DEBUG("Thread spawning complete.");
    
    return true;
}



int main(){
    bool spawnOne = false;
    while(true){
        if(!spawnOne){
	    DEBUG("BEGINNING SPAWNING PROCESS...");
            spawnNewBgt(0);
            spawnOne = true;
            DEBUG("SPAWNING PROCESS COMPLETE.");
	}
    }
    DEBUG("Loop Exited");
    return EXIT_SUCCESS; 
}
