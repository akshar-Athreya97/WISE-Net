using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Line : MonoBehaviour
{
    public GameObject C1;
    public GameObject C2;
    private float counter = 0f;
    private float dist;
    public float lineDrawSpeed;



    private LineRenderer line;


    // Use this for initialization
    void Start()
    {
        line = this.gameObject.AddComponent<LineRenderer>();
        line.SetWidth(0.05f, 0.05f);
        line.SetVertexCount(2);

        dist = Vector3.Distance(C1.transform.position, C2.transform.position);
    }

    // Update is called once per frame
    void Update()
    {
        if (counter < dist)
        {
            if (C1 != null && C2 != null)
            {
                line.SetPosition(0, C1.transform.position);
                counter += .1f / lineDrawSpeed;
                float x = Mathf.Lerp(0, dist, counter);

                Vector3 A = C1.transform.position;
                Vector3 B = C2.transform.position;

                Vector3 PAL = x * Vector3.Normalize(B - A) + A;

                line.SetPosition(1, PAL);
            }

        }
    }
}
   