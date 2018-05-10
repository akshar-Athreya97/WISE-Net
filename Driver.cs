/*
 * 13th July 2017
 * Basic Code for change of colors when data gets sent
 * Node IDs, Location and Path are all public variables which the user can put in 
 * Wait time between change of colors is also a Public Variable
 * Version 1.1
 */

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO.Ports;

public class Driver : MonoBehaviour {
    SerialPort sp = new SerialPort("\\\\.\\COM12", 9600);
    public string Path;
	/*	
	 * 0 --> BaseStation
	 * 	1 --> Node_1
	 *  2 --> Node_2
	 *  3 --> Node_3
     *  4 --> Node_4
     *  5 --> Node_5
     *  6 --> Node_6
	 */


	//Array Of GameObjects representing Nodes (inclusive of the BaseStation)
	public GameObject[] Nodes = new GameObject[20];


	//Different Materials for Different States of the Nodes
	public Material ActivatedMaterial;   // When Data gets sent Node turns green
	public Material DefaultMaterial;     // Default color of all Nodes and Base Station is Yellow
	public Material FailureMaterial;     // When there is loss of connection, Node Turns Red

    //TimeLapse between each Node State Change
    public float WaitTimeInSeconds;

	int NoOfNodes;

	void Start()
	{
        sp.Open();
        Path = sp.ReadLine();
        NoOfNodes = Nodes.Length;
		StartCoroutine (DriverMethod (WaitTimeInSeconds));
        //print(Path);
      
	}

	IEnumerator DriverMethod(float waitTime)
	{
        int l = Path.Length;
        int i = 0;
        while (i<l) {
            if (Path[i] == '+')
            {
                i++;
                print("Postive");
                int index = Path[i] - '0'; //Converting Character into integer
                print(index);
                for (int j = 0; j < NoOfNodes; j++)
                {
                    if (j == index) Nodes[j].GetComponent<Renderer>().material = ActivatedMaterial; // sets the node color to green
                    else Nodes[j].GetComponent<Renderer>().material = DefaultMaterial;              // sets it back to yellow ; Default state
                } 
                yield return new WaitForSecondsRealtime(waitTime); // Time Delay
               
            }
            else if (Path[i] == '-')
            {
                i++;
                print("Negative");
                int index = Path[i] - '0'; //Converting Character into integer
                print(index);
                
                for (int j = 0; j < NoOfNodes; j++)
                {
                    if (j == index) Nodes[j].GetComponent<Renderer>().material = FailureMaterial; // sets the node color to green
                    else Nodes[j].GetComponent<Renderer>().material = DefaultMaterial;              // sets it back to yellow ; Default state
                }
                yield return new WaitForSecondsRealtime(waitTime); // Time Delay
            }
            else 
                i++;
       }
	}
}
