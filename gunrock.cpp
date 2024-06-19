#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <deque>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "HttpService.h"
#include "HttpUtils.h"
#include "FileService.h"
#include "MySocket.h"
#include "MyServerSocket.h"
#include "dthread.h"

using namespace std;

int PORT = 8080;
int THREAD_POOL_SIZE = 1;
unsigned int BUFFER_SIZE = 1;
string BASEDIR = "static";
string SCHEDALG = "FIFO";
string LOGFILE = "/dev/null";

//create locks and condition variables
// lock called queueMutex;
// condition : hasRequest
// condition: hasSpace;
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hasRequests = PTHREAD_COND_INITIALIZER;
pthread_cond_t hasSpace = PTHREAD_COND_INITIALIZER;
typedef deque<MySocket*> SocketQueue;

//stores the requestqueue
SocketQueue clientQueue;


vector<HttpService *> services;

HttpService *find_service(HTTPRequest *request) {
   // find a service that is registered for this path prefix
  for (unsigned int idx = 0; idx < services.size(); idx++) {
    if (request->getPath().find(services[idx]->pathPrefix()) == 0) {
      return services[idx];
    }
  }
  return NULL;
}

//prints out what type of request and path needed
void logAndPrint(const string& type, const string& path) {
    stringstream payload;
    payload << type << " request for: " << path;
    sync_print("Method Invoked", "[" + payload.str() + "]");
    cout << payload.str() << endl;
}


void invoke_service_method(HttpService *service, HTTPRequest *request, HTTPResponse *response) {
    if (service == NULL) {
        response->setStatus(403); // Forbidden
        response->setBody("forbidden access");
    } else if (request->isHead()) {
        //logAndPrint("HEAD", request->getPath());
        service->head(request, response);
    } else if (request->isGet()) {
        //logAndPrint("GET", request->getPath());
        service->get(request, response);
    } else {
        response->setStatus(501);
    }
}



//read request
//decide what service to handle the request,
//send response signal back to parent thread
void *handle_request(MySocket *client) {
  HTTPRequest *request = new HTTPRequest(client, PORT);
  HTTPResponse *response = new HTTPResponse();
  stringstream payload;
  

  // read in the request
  bool readResult = false;
  try {
    payload << "client: " << (void *) client;
    sync_print("read_request_enter", payload.str());
    readResult = request->readRequest();
    sync_print("read_request_return", payload.str());
  } catch (...) {
    // swallow it
  }
    
  if (!readResult) {
    response->setStatus(403);
    client->write(response->response());
    delete response;
    delete request;
    sync_print("read_request_error", payload.str());
    return NULL;
}

  
  HttpService *service = find_service(request);
  invoke_service_method(service, request, response);

  // send data back to the client and clean up
  payload.str(""); payload.clear();
  payload << " RESPONSE " << response->getStatus() << " client: " << (void *) client;
  sync_print("write_response", payload.str());
  cout << payload.str() << endl;
  client->write(response->response());
    
  delete response;
  delete request;

  payload.str(""); payload.clear();
  payload << " client: " << (void *) client;
  sync_print("close_connection", payload.str());
  client->close();
  delete client;
  return NULL;
}


void *processClientRequests(void* arg) {
    while (true) {
        dthread_mutex_lock(&queueMutex);    //lock shared date
        while (clientQueue.empty()) { //if the queue is empty
            dthread_cond_wait(&hasRequests, &queueMutex);  //  wait until there are requests
        }
        MySocket* client = clientQueue.front(); //take first on queue
        clientQueue.pop_front();    // remove it from queue, opening up space on the queue

        dthread_cond_signal(&hasSpace);  // signal that theres space with conditionVariable
        dthread_mutex_unlock(&queueMutex);  // unlock shared data
        
        handle_request(client);  // Directly handle the request without checking for nullptr
    }
    return NULL;
}




int main(int argc, char *argv[]) {

  signal(SIGPIPE, SIG_IGN);
  int option;

  while ((option = getopt(argc, argv, "d:p:t:b:s:l:")) != -1) {
    switch (option) {
    case 'd':
      BASEDIR = string(optarg);
      break;
    case 'p':
      PORT = atoi(optarg);
      break;
    case 't':
      THREAD_POOL_SIZE = atoi(optarg);
      break;
    case 'b':
      BUFFER_SIZE = atoi(optarg);
      break;
    case 's':
      SCHEDALG = string(optarg);
      break;
    case 'l':
      LOGFILE = string(optarg);
      break;
    default:
      cerr<< "usage: " << argv[0] << " [-p port] [-t threads] [-b buffers]" << endl;
      exit(1);
    }
  }

  set_log_file(LOGFILE);

  sync_print("init", "");
  MyServerSocket *server = new MyServerSocket(PORT);
  MySocket *client;

  //the order that you push services dictates the search order
  //for path prefic matching
  services.push_back(new FileService(BASEDIR));
  

  std::vector<pthread_t> threadPool(THREAD_POOL_SIZE);

  for (auto& thread : threadPool) {
      if (pthread_create(&thread, NULL, processClientRequests, NULL) != 0) {
          perror("Failed to create a thread");
          break;
      }
  }

  while(true) {
        sync_print("waiting_to_accept", "");
        client = server->accept();
        sync_print("client_accepted", "");
        //handle_request(client);
        pthread_mutex_lock(&queueMutex);
        while (clientQueue.size() == BUFFER_SIZE) {
            pthread_cond_wait(&hasSpace, &queueMutex);
        }
        clientQueue.push_back(client);
        pthread_cond_signal(&hasRequests);
        pthread_mutex_unlock(&queueMutex);
    }
}

