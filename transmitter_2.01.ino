/*version 2.01
14 July 2017
base station code*/
#include <RF24Network_config.h>
#include<RF24Network.h>
#include<RF24.h>
#include<SPI.h>

#define base 0
#define node 1

RF24 radio(7,8);
RF24Network network(radio); 

struct payload_t     //payload to be sent
{
  byte node_id;
  byte parent_node;
  byte level;
  byte data;
  byte trig_type;
};
struct table       //forwarding table structure
{
  byte node_id;
  byte parent_node;
  byte level;
};

/*function declarations*/
payload_t listening(RF24NetworkHeader header);
void payload_receive(struct payload_t* payload);
void pingback_function(payload_t payload );
void make_payload(struct payload_t* payload, byte trig_type);
void display_payload(payload_t payload);
void broadcast();
void neighbor_recognition(struct payload_t *payload);
void just_forwarding(struct payload_t *payload);
void display_neighbors();
void transfer_control(payload_t payload);

/*variable declarations*/
byte this_node=00;
byte node_type=base;
byte level=(this_node==00?0:99);       //ternary operator
byte parent_node=255;
bool found=0;                          //to check if node exists already in neighbors
bool found1=0;                         //to check if node exists already in leaves
uint8_t top=0;                         //counter for where the neighbors stack ends
uint8_t top1=0;                        //counter for where the leaves stack ends                
uint8_t table_top=0;                   //counter for where the forwarding table ends
payload_t neighbors[10];               //array containing each of the neighbors' payloads
payload_t leaves[10];
bool level_set=0;                      // flag to check if level has been set
int neighbor_number=0;                 //counter used to give each neighbor control to broadcast
bool broadcast_status=false;
table fwd_table[20];

void setup() 
{
  Serial.begin(9600);
  SPI.begin();
  radio.begin();
  network.begin(90,this_node);        //setting up the network with the node_id
}

void loop() 
{
  if (!broadcast_status)
  {
    broadcast();
    broadcast_status=true;    //giving the initial broadcast
  }
  RF24NetworkHeader header;
  long int init=millis();
  payload_t payload;
  payload=listening(header);   //node keeps listening and checks which trig_type
  //payload is the payload that it receives from the child
  switch(payload.trig_type)
  {
    case 1:    //triggers node to transfer broadcasting control to its child nodes
    {
      neighbor_recognition(&payload); //when we get ping back from child nodes, neighbors array is created
      if(neighbor_number==0)            //Transmit the control packet only first time you receive the ping back to the first child
      {
        transfer_control(neighbors[neighbor_number]);  //transfers control to the first neighbor
      }
      break;
    }
    
    case 3:  //leaf node has been reached.
    {
      found1=0;
      Serial.println("Received leaf node from: ");
      display_payload(payload);
      for(int i=0; i<top1; i++)
      {
        if(leaves[i].node_id==payload.node_id) //this means the node has already been recognized by the base
        {
           found1=1;
           break;
        }
      }//end of for()
      if(!found1)
      {
        leaves[top1++]= payload;  //gets other parent nodes which will be usd as alternate routes- it will exclude parent node  
      }//end of if
      //neighbor_number ensures that broadcast happens one after the other
      flag:
      if(neighbor_number<top)
      {
        if(neighbors[neighbor_number].level>level)      //if the level of the neighbor is higher than this node's level, then transfer control
        {
          transfer_control(neighbors[neighbor_number]);
        }
        else
        { 
          neighbor_number++;
          goto flag;
        }
      }
      else if(neighbor_number==top)
      {
        
      }
      break;
    }
    case 4:
    {
      Serial.println("Got data from");
      display_payload(payload);
      Serial.println("Temperature: ");
      float cel=(payload.data/1024.0)*500;
      Serial.print(cel);
      Serial.print(" *C");
      Serial.println();
      Serial.println();
      int found2=0;
      for(int i=0; i<table_top; i++)
      {
        if(fwd_table[i].node_id==payload.node_id) //this means the node has already been recognized by the base
        {
           found2=1;
           break;
        }
      }//end of for()
      if(!found2)
      {
        fwd_table[table_top].node_id= payload.node_id;  //gets other parent nodes which will be usd as alternate routes- it will exclude parent node  
        fwd_table[table_top].parent_node=payload.parent_node;
        fwd_table[table_top].level=payload.level;
        table_top++; 
        for(int i=0; i<table_top; i++)
        {
          for(int j=0; j<top; j++)
          {
            if(fwd_table[i].node_id==neighbors[j].node_id)
            {
              fwd_table[table_top].node_id=this_node;
              fwd_table[table_top].parent_node=parent_node;
              fwd_table[table_top].level=level;
              table_top++;
              break;
            }
          }
        }
      }//end of if
      break;      
    }
    default:
    {
      delay(100);
      break;
    } 
  }//end of switch
  if(top1==top)
  {
    Serial.println();
    Serial.println("The forwarding table is: ");
    for(int i=0; i<table_top-1; i++)
    {
      Serial.println("Node ID: ");
      Serial.println(fwd_table[i].node_id);
    }
  }
  delay(10);
}//end of loop()


payload_t listening(RF24NetworkHeader header)
{
/*just keeps listening */
  network.update();
  struct payload_t payload;
  bool ret=0;
  while(network.available())
  {
    ret=network.read(header, &payload, sizeof(payload)); //read whatever data base station is sending 
    network.update();
    delay(100);
    return payload;
  }//end of if
  if(!ret)                               
  {
    payload.trig_type=99;
    return payload;
  }
}//end of listening()



void make_payload(struct payload_t* payload, byte trig_type)
{
/*Preparing payload for transmission adding node id, node level, node type and data......BTW it uses a pointer Be carefull :P */
  payload->node_id=this_node;
  payload->parent_node=parent_node;
  payload->level=level;
  payload->data=0;  //We can worry about this later
  payload->trig_type=trig_type;  
}//end of make_payload()

void display_payload(payload_t payload)
{
/*to display node id and level*/
  Serial.println("Node ID: ");
  Serial.println(payload.node_id);
  Serial.println("Level: ");
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
  Serial.println(payload.trig_type);
  long initial=millis();
  while(millis()-initial<3000)      //broadcasts for 1 second
  { 
    Serial.println("Sending...");
    network.multicast(header,&payload,sizeof(payload),1);
    network.update();
    delay(200);
  } //end of while()
  network.update(); 
}//end of broadcast()


void neighbor_recognition(struct payload_t *payload)
{
  network.update();
  //display_payload(*payload);
  delay(100);
  uint8_t found=0;
  for(int i=0; i<top; i++)
  {
    if(neighbors[i].node_id==payload->node_id) //this means the node has already been recognized by the base
    {
       found=1;
       break;
    }
  }//end of for()
  if(!found)
  {
    neighbors[top++]= *payload;  //gets other parent nodes which will be usd as alternate routes- it will exclude parent node  
  }//end of if
}//end of neighbor_recognition


void transfer_control(payload_t payload)
{
  /*transfers control to broadcast to the child node*/
  neighbor_number++;
  Serial.println("Transferring control to: ");
  display_payload(payload);
  payload_t broadcastctrl_payload;
  make_payload(&broadcastctrl_payload,2);
  RF24NetworkHeader header2(payload.node_id);
  network.write(header2, &broadcastctrl_payload, sizeof(payload_t));
  network.update();
}

void display_neighbors()
{
  Serial.println("The neighbors are: ");
  for(int i=0; i<top; i++)
  {
    if(neighbors[i].level<level)
      Serial.println("Parent node: ");
    else if(neighbors[i].level==level)
      Serial.println("Sibling node: ");
    else
      Serial.println("Child node: ");
    display_payload(neighbors[i]);
    delay(100);
  }
}

/*void just_forwarding(struct payload_t *payload)
{
  //to forward from leaf node
  RF24NetworkHeader header(parent_node);
  network.write(header, payload, sizeof(payload_t));
  delay(10);
}//end of just_forwarding()*/


//errors- if a node doesn't receivea broadcast for long, we havent programmed it to send the leaf packet 

