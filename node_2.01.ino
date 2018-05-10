/*version 2.01
19 July 2017
node code*/
#include <RF24Network_config.h>
#include<RF24Network.h>
#include<RF24.h>
#include<SPI.h>

#define base 0
#define node 1

RF24 radio(7,8);`
RF24Network network(radio); 

struct payload_t     //payload to be sent
{
  byte node_id;
  byte parent_node;
  byte level;
  byte data;
  byte trig_type;
};

/*function declarations*/
payload_t listening(RF24NetworkHeader header);
void payload_receive(struct payload_t payload);
void pingback_function(payload_t payload,payload_t this_payload );
void make_payload(struct payload_t* payload, byte trig_type);
void display_payload(payload_t payload);
void broadcast();
void neighbor_recognition(struct payload_t *payload);
bool just_forwarding(struct payload_t *payload,byte parent);
void display_neighbors();
void transfer_control(payload_t payload);

/*variable declarations*/
byte this_node=01;
byte node_type=node;
byte level=(this_node==00?0:99);      //ternary operator
byte parent_node=255;
byte leaf=0;  
bool found=0;                          //to check if node exists already
uint8_t top=0;                         //counter for where the stack ends
payload_t neighbors[10];               //array containing each of the neighbors' payloads
int row=0;                             //rows have each of the lines
int col=0;                             //columns have each of the nodes' payloads
bool level_set=0;                      // flag to check if level has been set
bool pingback=0;
bool data=0;
int neighbor_number=0;             //this is counter used to give each neighbor control to broadcast
int tempPin=A0;                       //Pin to read data from sensor
long check_leaf=0;
bool broadcast_status=false;
payload_t this_payload1;
void setup() 
{
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(90,this_node);
  make_payload(&this_payload1,3);  //to ensure that previous trig_type is removed and it gets reinitialised
}

void loop() 
{
  RF24NetworkHeader header;
  long init2=millis();
  payload_t payload;
  payload_t this_payload;
  make_payload(&payload,99);
  payload=listening(header);   //node keeps listening and checks which trig_type
  //payload consists of the data from the parent node
  switch(payload.trig_type)
  {
    case 0:    //triggers node to ping back with trig_type 1
    {
      pingback=1;                   //this ensures that the node has received something from parent before it checks whether it is a leaf node
      payload_receive(payload);     //level gets set
      make_payload(&this_payload,1); //going to send back my node id to the base along with changed trig_type
      pingback_function(payload, this_payload ); //pings back to everyone only from a lower level
      Serial.println("node_id");
      Serial.println(this_payload.node_id);
      Serial.println(this_payload.level);
      break;
    }
    case 1:                                                   //triggers node to transfer broadcasting control to its child nodes-creates neighbor array
                                                              //doesnt do anything if it receives 1 from a sibling node
    {       
      if(payload.level<level)
      {                        
        payload_receive(payload);                              //level gets set and 
      }
      if (payload.level>level&& broadcast_status== true)       // broadcast control can be given to only child nodes
      {
        leaf=1;                                                //means it is not a leaf node
        neighbor_recognition(&payload);
        if(neighbor_number==0)
        {
          flag1:
          if(neighbors[neighbor_number].level>level)           //to make sure parent nodes dont receive broadcast control
            transfer_control(neighbors[neighbor_number]);      //only transfers control to the first child node
          else
          {
            neighbor_number++;
            goto flag1;
          }
        }
      }
      break;
    }
    case 2:                                                  //This node has received control to broadcast. Triggers a broadcast to find child nodes
    {
      if(!broadcast_status)
      {
        broadcast();
        broadcast_status=true;      //to decide whether it should do anything when it receives pingback broadcast from a higher level
      }
      else
      {
        payload_t send_3_payload;
        make_payload(&send_3_payload,3);       
        just_forwarding(&send_3_payload,payload.node_id);
      }
      check_leaf=millis();
      break;
    }
    case 3:                        //leaf node has been reached.
    {
      flag:
      Serial.println("In case 3: ");
      //Serial.println(top);
      //Serial.println(neighbor_number);
      if(neighbor_number<top)                                //now, one branch is done, needs to transfer control
      {
        if(neighbors[neighbor_number].level>level)
        {
          transfer_control(neighbors[neighbor_number]);
        }
        else      //if there are neighbors in the same level
        {
          neighbor_number++;
          goto flag;
        }
      }
      else if(neighbor_number==top)
      {
        bool written=0;
        payload_t payload1;
        make_payload(&payload1,3);       
        while(!written)
        {
          Serial.println("received a leaf transferring to base");
          written=just_forwarding(&payload1,parent_node);
        }
        Serial.println ("don transferring type 3");
        make_payload(&payload1,4);
        just_forwarding(&payload1,parent_node);
        Serial.println ("don transferring type 4");
      }
      break;
    }
    case 4:
    {
      just_forwarding(&payload,parent_node);    //this contains the forwarding table data
      Serial.println("Sending data of this node ");
      make_payload(&this_payload,4);
      just_forwarding(&this_payload,parent_node);
      break;
    }
    default:
    {
      if (leaf==0  && pingback==1 && check_leaf!=0)                                  //if its a leaf node it will go into default and send only one leaf packet to its parent
      {
        leaf=1;                                                                   //Each node must only transmit one leaf node
        data=1;
        int written=0;
        Serial.println("This is a leaf Node");
        
        make_payload(&this_payload,3);
       
        Serial.println("Sending a leaf packet");
        written=just_forwarding(&this_payload,parent_node);
      
       
        make_payload(&this_payload,4);
        just_forwarding(&this_payload,parent_node);
        display_neighbors();
      }
      if(data)
      {
        Serial.println("Sending Data");
        make_payload(&this_payload,4);
        just_forwarding(&this_payload,parent_node); 
      }
      delay(100);
      break;
    } 
  }//end of switch
  delay(10);
}//end of loop()


payload_t listening(RF24NetworkHeader header)
{
  /*just keeps listening */
  struct payload_t payload;
  bool ret=0;    //this is to ensure a dummy value is not initialised ad it does not go into the switch case
  network.update();
  while(network.available())
  {
    ret=network.read(header, &payload, sizeof(payload_t)); //read whatever data base station is sending
    network.update(); 
    delay(100);
    return payload; 
  }//end of while
  if(!ret)
     payload.trig_type=99;
  return payload;
}//end of listening()

void payload_receive(struct payload_t payload)
{
/*function to set level of the node 
keeps track of parent node
appends any other parent nodes into neighbors*/
  network.update();
  if(level > payload.level+1)      //+1 so that level doesnt keep incrementing 
  {
     level=payload.level+1;              //level increases according to level of whichever node sent payload
     Serial.println("from lower level");
     display_payload(payload);
     parent_node=payload.node_id; //setting the parent node- first node it receives broadcast from
  }//end of if
  neighbor_recognition(&payload);    //when we get ping back from child nodes, creates array of neighbors
}//end of payload_receive()

void pingback_function(payload_t payload, payload_t this_payload)
{
  
  /*pinging back to the parent node*/
  if (payload.level<level)   //it ensures that node doesnt ping back to other nodes in same level
  {
      RF24NetworkHeader header1(payload.node_id);   //header to write
      RF24NetworkHeader header2;   //header to multicast
      Serial.println("Pinging back");
      network.write(header1,&this_payload,sizeof(payload_t));      //writes back this nodes details
      network.multicast(header2,&this_payload,sizeof(payload_t),1);
      network.update();
      delay(100);
  } 
  network.update();      
}//end of pingback_function()


void make_payload(struct payload_t* payload, byte trig_type)
{
/*Preparing payload for transmission adding node id, node level, node type and data......BTW it uses a pointer Be carefull :P */
  
  payload->node_id=this_node;
  payload->parent_node=parent_node;
  payload->level=level;
  byte val;
  val = analogRead(tempPin);
   float mv = ( val/1024.0)*5000; 
  float cel = mv/10;
  float farh = (cel*9)/5 + 32;
  
  //Serial.print("TEMPERATURE = ");
  //Serial.print(cel);
  //Serial.print("*C");
  //Serial.println();
 // delay(1000);
  payload->data=val;
  payload->trig_type=trig_type;  
}//end of make_payload()

void display_payload(payload_t payload)
{
/*to display node id and level*/
  Serial.println("Node_id: ");
  Serial.println(payload.node_id);
  Serial.println("At level: ");
  Serial.println(payload.level);
  delay(500);
}//end of display_payload()

void broadcast()
{
/*function to broadcast from this node */
  RF24NetworkHeader header;
  struct payload_t payload;
  make_payload(&payload,0);
  network.update();    
  
  long initial=millis();
  while(millis()-initial<3000)      //broadcasts for 1 seconds
  { 
    Serial.println("received control to broadcast, sending");
    network.multicast(header,&payload,sizeof(payload_t),1);
    network.update();
    delay(200);
  }//end of while()
  network.update(); 
}//end of broadcast()


void neighbor_recognition(struct payload_t *payload)
{
  /*creates an array containing all neighbors*/
  network.update();
  display_payload(*payload);
  uint8_t found=0;
  for(int i=0; i<top; i++)
  {
    if(neighbors[i].node_id==payload->node_id) //this means the node has already been recognized by the parent
    {
       found=1;
       break;
    }
  }//end of for()
  if(!found)
  {
    neighbors[top++]= *payload; //now neighbours has payload of all of this nodes children + its parents
  }
}//end of neighbor_recognition


void transfer_control(payload_t payload)
{
  /*transfers control(trig_type 2) to one child depending on neighbour_number*/
  Serial.println("Transferring control to: ");
  display_payload(payload);
  payload_t broadcastctrl_payload;
  make_payload(&broadcastctrl_payload,2);
  RF24NetworkHeader header2(payload.node_id);
  network.write(header2, &broadcastctrl_payload, sizeof(payload_t));
  network.update();
  neighbor_number++;
}

bool just_forwarding(struct payload_t *payload,byte parent)
{
  /*to forward from leaf node*/
  
  int pos=0;
  RF24NetworkHeader header(parent);
  bool written=0;
  written =network.write(header, payload, sizeof(payload_t));
  //Serial.println(written);
  network.update();
  delay(100);
  return written;
 /* write_again:
  if(payload->trig_type==3)
  {
    written=network.write(header, payload, sizeof(payload_t));
    network.update();
    if(!written)
      goto write_again;
  }
  if(payload->trig_type==4)
  {
    if(!written)
    {
      for(int i=pos; i<top; i++)
      {
        if(neighbors[i].level==level-1)
        {
          RF24NetworkHeader header(neighbors[i].node_id);
          written=network.write(header, payload, sizeof(payload_t));
          network.update();
          pos=i;
          goto write_again;
        }
      }
    }
  }*/
}//end of just_forwarding()

void display_neighbors()
{
  Serial.println("the neighbours are: ");
  for (int i=0; i<top; i++)
  {
    if (neighbors[i].level <level)
      Serial.println("Parent node: ");
    else if (neighbors[i].level ==level)
      Serial.println("Sibling node: ");
    else if (neighbors[i].level>level)
      Serial.println("child node: ");
    display_payload(neighbors[i]);
  }
  delay(500);
}
//errors- if a node doesn't receive a broadcast for long, we havent programmed it to send the leaf packet 
//neighbours array has to be sorted

