/*
 * 13th July 2017
 * Code for animated Drawing of lines between 2 Nodes(excluding Base Station)
 * Version 1.1
 */

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Node2Node : MonoBehaviour
{
    public GameObject C1;       //Any gameObject part of the scene
    public GameObject C2;       // 

    private float counter = 0f; // Counter is used to animate the line and it adds to the distance the line is drawn for
    private float dist;         // distance between the 2 objects which the line connects
    public float lineDrawSpeed; // Speed at which the line is drawn
    public float time;          // Time delay 



    private LineRenderer line;  //variable of type LineRenderer used to draw and give properties to line


    // Use this for initialization
    void Start()
    {
        line = this.gameObject.AddComponent<LineRenderer>(); // Adding LineRenderer component to the variable line
        line.SetWidth(0.05f, 0.05f);    //Sets the Line Width
        line.SetVertexCount(2);         // ??

        dist = Vector3.Distance(C1.transform.position, C2.transform.position); // used to check the distance between objects C1 and C2 in vector 3D form
    }

    // Update is called once per frame
    void Update()
    {
        StartCoroutine("drawline"); // Calls the drawLine function
    }
    //IEnumerator is a function used for Time Delay
    IEnumerator drawline()
    {
        if (counter < dist) // count cannot be greater than distance otherwise cannot draw the line
        {
            if (C1 != null && C2 != null) //  Both objects need to exist in the scene
            {
                yield return new WaitForSeconds(time); // Delay for how long 

                line.SetPosition(0, C1.transform.position); // Origin position of the line

                counter += .1f / lineDrawSpeed; // appending the counter till dist is reached

                float x = Mathf.Lerp(0, dist, counter);

                Vector3 A = C1.transform.position; // gives the position of C1 in the scene and puts it in A

                Vector3 B = C2.transform.position; // gives the position of C2 in the scene and puts it in B

                Vector3 PAL = x * Vector3.Normalize(B - A) + A; // any point along the line and this equation is used to animate the line

                line.SetPosition(1, PAL); // Final position of line
            }

        }
    }
}