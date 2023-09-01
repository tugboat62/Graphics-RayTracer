#define _USE_MATH_DEFINES

#include <iostream>
#include <stdio.h>
#include <windows.h> // for MS Windows
#include <GL/glut.h> // GLUT, include glu.h and gl.h
#include <cmath>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "bitmap_image.hpp"
#include "1805051_Header.h"

using namespace std;

bitmap_image image;
vector<Light> normal_lights;
vector<SpotLight> spot_lights;
vector<Object *> objects;

struct point pos(0, -200, 35); // position of the eye
struct point l;                // look/forward direction
struct point r;                // right direction
struct point u;                // up direction

GLfloat near_plane;
GLfloat far_plane;
GLfloat fov, aspect_ratio, checkerboard;
GLfloat ka, kd, kr;
int pixel_size, no_objects, normal_light, spot_light;
int numTiles = 20;
float windowWidth = 4;
float windowHeight = 4;
int imageCount = 1;
int recursion_level;

float gridCenterX = 0.0f;
float gridCenterY = 0.0f;

/* Initialize OpenGL Graphics */
void initGL()
{
    // Set "clearing" or background color
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black and opaque
    glEnable(GL_DEPTH_TEST);              // Enable depth testing for z-culling
}

// Global variables

float angle = 0.0;   // Rotation angle for animation
bool rotate = false; // Rotate triangle?
int drawgrid = 0;    // Toggle grids

void drawAxes()
{
    glLineWidth(3);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); // Red
    // X axis
    glVertex3f(0, 0, 0);
    glVertex3f(2, 0, 0);

    glColor3f(0, 1, 0); // Green
    // Y axis
    glVertex3f(0, 0, 0);
    glVertex3f(0, 2, 0);

    glColor3f(0, 0, 1); // Blue
    // Z axis
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 2);
    glEnd();
}

void display()
{
    // glClear(GL_COLOR_BUFFER_BIT);            // Clear the color buffer (background)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); // To operate on Model-View matrix
    glLoadIdentity();           // Reset the model-view matrix

    // default arguments of gluLookAt
    // gluLookAt(0,0,0, 0,0,-100, 0,1,0);

    // control viewing (or camera)
    gluLookAt(pos.x, pos.y, pos.z,
              pos.x + l.x, pos.y + l.y, pos.z + l.z,
              u.x, u.y, u.z);

    glRotated(angle, 0, 1, 0);

    gridCenterX = -pos.x; // Inverse translation for grid center
    gridCenterY = -pos.y;

    for (int i = 0; i < objects.size(); i++)
    {
        objects[i]->draw();
    }

    for (int i = 0; i < normal_lights.size(); i++)
    {
        normal_lights[i].draw();
    }

    for (int i = 0; i < spot_lights.size(); i++)
    {
        spot_lights[i].draw();
    }

    // drawAxes();

    glutSwapBuffers(); // Render now
}

void capture()
{
    cout<<"Capturing Image"<<endl;

    image = bitmap_image(pixel_size, pixel_size);

	// initialize bitmap image and set background color to black
	for(int i=0;i<pixel_size;i++)
		for(int j=0;j<pixel_size;j++)
			image.set_pixel(i, j, 0, 0, 0);
	
	// image.save_image("black.bmp");

	windowHeight  = 2*(near_plane * tan((M_PI * fov/2) / 360.0));
    windowWidth = windowHeight * aspect_ratio;

	point topLeft = pos + (l * near_plane) + (u * (windowHeight / 2.0)) - (r * (windowWidth / 2.0));

	double du = windowWidth / (pixel_size*1.0);
	double dv = windowHeight / (pixel_size*1.0);

	// Choose middle of the grid cell
	topLeft = topLeft + (r * du / 2.0) - (u * dv / 2.0);

	int nearestObjectIndex = -1;
	double t,tMin;

	for(int i=0;i<pixel_size;i++)
	{
		for(int j=0;j<pixel_size;j++)
		{
			// calculate current pixel
			point pixel = topLeft + (r * du * i) - (u * dv * j);

			// cast ray from EYE to (curPixel-eye) direction ; eye is the position of the camera
			Ray ray(pos,pixel-pos);
			point color;

			// cout<<"Ray direction "<<ray.dir<<endl;

			// find nearest object
			tMin = -1;
			nearestObjectIndex = -1;
			for(int k=0;k<(int)objects.size();k++)
			{
				t = objects[k]->intersect(ray,color, 0);
				if(t>0 && (nearestObjectIndex == -1 || t<tMin) )
					tMin = t , nearestObjectIndex = k;
			}

			// if nearest object is found, then shade the pixel
			if(nearestObjectIndex != -1)
			{
				// cout<<"Object "<<nearestObjectIndex<<" intersected"<<endl;
				// color = objects[nearestObjectIndex]->color;
				color = point(0,0,0);
				// cout<<"Before Color "<<color.x<<" "<<color.y<<" "<<color.z<<endl;
				double t = objects[nearestObjectIndex]->intersect(ray,color, 1);

				if(color.x > 1) color.x = 1;
				if(color.y > 1) color.y = 1;
				if(color.z > 1) color.z = 1;

				if(color.x < 0) color.x = 0;
				if(color.y < 0) color.y = 0;
				if(color.z < 0) color.z = 0;
				
				// cout<<"After Color "<<color.x<<" "<<color.y<<" "<<color.z<<endl;
				image.set_pixel(i, j, 255*color.x, 255*color.y, 255*color.z);
			}
		}
	}

	image.save_image("Output_"+to_string(imageCount)+".bmp");
	imageCount++;
	cout<<"Saving Image"<<endl;
}

/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshapeListener(GLsizei width, GLsizei height)
{ // GLsizei for non-negative integer
    // Compute aspect ratio of the new window
    if (height == 0)
        height = 1; // To prevent divide by 0

    // Set the viewport to cover the new window
    glViewport(0, 0, width, height);

    // Set the aspect ratio of the clipping area to match the viewport
    glMatrixMode(GL_PROJECTION); // To operate on the Projection matrix
    glLoadIdentity();            // Reset the projection matrix
    /*if (width >= height) {
        // aspect >= 1, set the height from -1 to 1, with larger width
        gluOrtho2D(-1.0 * aspect, 1.0 * aspect, -1.0, 1.0);
    } else {
        // aspect < 1, set the width to -1 to 1, with larger height
        gluOrtho2D(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect);
    }*/
    // Enable perspective projection with fovy, aspect, zNear and zFar
    gluPerspective(fov, aspect_ratio, near_plane, far_plane);
}

/* Callback handler for normal-key event */
void keyboardListener(unsigned char key, int x, int y)
{
    double rate = 0.01;
    double v = 0.25;
    double s;
    switch (key)
    {
    // Rotation
    case '1':
        // look left
        r.x = r.x * cos(rate) + l.x * sin(rate);
        r.y = r.y * cos(rate) + l.y * sin(rate);
        r.z = r.z * cos(rate) + l.z * sin(rate);

        l.x = l.x * cos(rate) - r.x * sin(rate);
        l.y = l.y * cos(rate) - r.y * sin(rate);
        l.z = l.z * cos(rate) - r.z * sin(rate);
        break;
    case '2':
        // look right
        r.x = r.x * cos(-rate) + l.x * sin(-rate);
        r.y = r.y * cos(-rate) + l.y * sin(-rate);
        r.z = r.z * cos(-rate) + l.z * sin(-rate);

        l.x = l.x * cos(-rate) - r.x * sin(-rate);
        l.y = l.y * cos(-rate) - r.y * sin(-rate);
        l.z = l.z * cos(-rate) - r.z * sin(-rate);
        break;
    case '3':
        // look up
        l.x = l.x * cos(rate) + u.x * sin(rate);
        l.y = l.y * cos(rate) + u.y * sin(rate);
        l.z = l.z * cos(rate) + u.z * sin(rate);

        u.x = u.x * cos(rate) - l.x * sin(rate);
        u.y = u.y * cos(rate) - l.y * sin(rate);
        u.z = u.z * cos(rate) - l.z * sin(rate);
        break;
    case '4':
        // look down
        l.x = l.x * cos(-rate) + u.x * sin(-rate);
        l.y = l.y * cos(-rate) + u.y * sin(-rate);
        l.z = l.z * cos(-rate) + u.z * sin(-rate);

        u.x = u.x * cos(-rate) - l.x * sin(-rate);
        u.y = u.y * cos(-rate) - l.y * sin(-rate);
        u.z = u.z * cos(-rate) - l.z * sin(-rate);
        break;
    case '5':
        // tilt counterclockwise
        u.x = u.x * cos(rate) + r.x * sin(rate);
        u.y = u.y * cos(rate) + r.y * sin(rate);
        u.z = u.z * cos(rate) + r.z * sin(rate);

        r.x = r.x * cos(rate) - u.x * sin(rate);
        r.y = r.y * cos(rate) - u.y * sin(rate);
        r.z = r.z * cos(rate) - u.z * sin(rate);
        break;
    case '6':
        // tilt clockwise
        u.x = u.x * cos(-rate) + r.x * sin(-rate);
        u.y = u.y * cos(-rate) + r.y * sin(-rate);
        u.z = u.z * cos(-rate) + r.z * sin(-rate);

        r.x = r.x * cos(-rate) - u.x * sin(-rate);
        r.y = r.y * cos(-rate) - u.y * sin(-rate);
        r.z = r.z * cos(-rate) - u.z * sin(-rate);
        break;
    case 'a':
        // rotate obj clockwise
        angle -= 5;
        break;
    case 'd':
        // rotate obj counterclockwise
        angle += 5;
        break;
    case '.':
        drawgrid = 1 - drawgrid;
        break;
    case '0':
        capture();
        break;

    // Control exit
    case 27:     // ESC key
        exit(0); // Exit window
        break;
    }
    glutPostRedisplay(); // Post a paint request to activate display()
}

/* Callback handler for special-key event */
void specialKeyListener(int key, int x, int y)
{
    switch (key)
    {
    // Translation
    case GLUT_KEY_LEFT:
        pos.x -= r.x;
        pos.y -= r.y;
        pos.z -= r.z;
        break;
    case GLUT_KEY_RIGHT:
        pos.x += r.x;
        pos.y += r.y;
        pos.z += r.z;
        break;
    case GLUT_KEY_UP:
        pos.x += l.x;
        pos.y += l.y;
        pos.z += l.z;
        break;
    case GLUT_KEY_DOWN:
        pos.x -= l.x;
        pos.y -= l.y;
        pos.z -= l.z;
        break;
    case GLUT_KEY_PAGE_UP:
        pos.x += u.x;
        pos.y += u.y;
        pos.z += u.z;
        break;
    case GLUT_KEY_PAGE_DOWN:
        pos.x -= u.x;
        pos.y -= u.y;
        pos.z -= u.z;
        break;
    default:
        return;
    }
    glutPostRedisplay(); // Post a paint request to activate display()
}

void readFile()
{
    ifstream file;
    file.open("description.txt");
    if (!file)
    {
        cout << "Unable to open file";
        exit(1); // terminate with error
    }
    string line;
    getline(file, line);

    istringstream iss(line);
    string token;

    float coord[4];
    int j = 0;
    while (iss >> token)
    {
        float number = stod(token);
        coord[j] = number;
        j++;
    }
    near_plane = coord[0];
    far_plane = coord[1];
    fov = coord[2];
    aspect_ratio = coord[3];

    getline(file, line);
    recursion_level = stoi(line);

    getline(file, line);
    pixel_size = stoi(line);
    getline(file, line);

    getline(file, line);
    checkerboard = stod(line);

    getline(file, line);
    istringstream iss2(line);
    j = 0;
    while (iss2 >> token)
    {
        float number = stod(token);
        coord[j] = number;
        j++;
    }
    ka = coord[0];
    kd = coord[1];
    kr = coord[2];
    Object *floor;
    floor = new Floor(checkerboard);
    objects.push_back(floor);
    floor->setCoEfficients( ka, kd, -1, kr);

    getline(file, line);
    getline(file, line);
    no_objects = stoi(line);
    getline(file, line);

    float tokens[13];

    while (getline(file, line))
    {
        if (line.compare("cube") == 0)
        {
            j = 0;
            for (int i = 0; i < 5; i++)
            {
                getline(file, line);
                istringstream iss3(line);
                while (iss3 >> token)
                {
                    float number = stod(token);
                    // cout << number << endl;
                    tokens[j] = number;
                    // cout << tokens[j] << endl;
                    j++;
                }
            }
            Object *s1, *s2, *s3, *s4, *s5, *s6;
            point reference(tokens[0], tokens[1], tokens[2]);
            point A(0, tokens[3], 0);
            point B(0, tokens[3], tokens[3]);
            point C(tokens[3], tokens[3], tokens[3]);
            point D(tokens[3], tokens[3], 0);
            point E(0, 0, 0);
            point F(0, 0, tokens[3]);
            point G(tokens[3], 0, tokens[3]);
            point H(tokens[3], 0, 0);
            point color(tokens[4], tokens[5], tokens[6]);
            int shine = (int)tokens[11];
            s1 = new square(A + reference, B + reference, C + reference, D + reference);
            s2 = new square(E + reference, F + reference, G + reference, H + reference);
            s3 = new square(E + reference, A + reference, B + reference, F + reference);
            s4 = new square(F + reference, B + reference, C + reference, G + reference);
            s5 = new square(G + reference, C + reference, D + reference, H + reference);
            s6 = new square(H + reference, D + reference, A + reference, E + reference);
            s1->setReferencePoint(reference);
            s2->setReferencePoint(reference);
            s3->setReferencePoint(reference);
            s4->setReferencePoint(reference);
            s5->setReferencePoint(reference);
            s6->setReferencePoint(reference);
            double ka = tokens[7];
            double kd = tokens[8];
            double ks = tokens[9];
            double kr = tokens[10];
            s1->setCoEfficients(ka, kd, ks, kr);
            s2->setCoEfficients(ka, kd, ks, kr);
            s3->setCoEfficients(ka, kd, ks, kr);
            s4->setCoEfficients(ka, kd, ks, kr);
            s5->setCoEfficients(ka, kd, ks, kr);
            s6->setCoEfficients(ka, kd, ks, kr);
            s1->setColor(color);
            s2->setColor(color);
            s3->setColor(color);
            s4->setColor(color);
            s5->setColor(color);
            s6->setColor(color);
            s1->setShine(shine);
            s2->setShine(shine);
            s3->setShine(shine);
            s4->setShine(shine);
            s5->setShine(shine);
            s6->setShine(shine);
            objects.push_back(s1);
            objects.push_back(s2);
            objects.push_back(s3);
            objects.push_back(s4);
            objects.push_back(s5);
            objects.push_back(s6);
        }
        else if (line.compare("sphere") == 0)
        {
            j = 0;
            for (int i = 0; i < 5; i++)
            {
                getline(file, line);
                istringstream iss4(line);
                while (iss4 >> token)
                {
                    float number = stod(token);
                    tokens[j] = number;
                    j++;
                }
            }
            Object *s;
            point center(tokens[0], tokens[1], tokens[2]);
            s = new sphere(center, tokens[3]);
            point color(tokens[4], tokens[5], tokens[6]);
            s->setReferencePoint(center);
            s->setColor(color);
            s->setCoEfficients(tokens[7], tokens[8], tokens[9], tokens[10]);
            s->setShine((int)tokens[11]);
            objects.push_back(s);
        }
        else if (line.compare("pyramid") == 0)
        {
            j = 0;
            for (int i = 0; i < 5; i++)
            {
                getline(file, line);
                istringstream iss5(line);
                while (iss5 >> token)
                {
                    float number = stod(token);
                    tokens[j] = number;
                    j++;
                }
            }
            // , *s
            Object *t1, *t2, *t3, *t4, *s;
            point reference(tokens[0], tokens[1], tokens[2]);
            double width = tokens[3];
            double height = tokens[4];
            point color(tokens[5], tokens[6], tokens[7]);
            point A(0, 0, 0);
            point B(width, 0, 0);
            point C(width, width, 0);
            point D(0, width, 0);
            point E(width/2.0, width/2.0, height);
            int shine = (int)tokens[12];

            t1 = new triangle(A + reference, C + reference, B + reference);
            t2 = new triangle(A + reference, B + reference, E + reference);
            t3 = new triangle(A + reference, C + reference, D + reference);
            t4 = new triangle(A + reference, D + reference, E + reference);
            s = new square(B + reference, C + reference, D + reference, E + reference);

            t1->setCoEfficients(tokens[8], tokens[9], tokens[10], tokens[11]);
            t2->setCoEfficients(tokens[8], tokens[9], tokens[10], tokens[11]);
            t3->setCoEfficients(tokens[8], tokens[9], tokens[10], tokens[11]);
            t4->setCoEfficients(tokens[8], tokens[9], tokens[10], tokens[11]);
            s->setCoEfficients(tokens[8], tokens[9], tokens[10], tokens[11]);

            s->setColor(color);
            t1->setColor(color);
            t2->setColor(color);
            t3->setColor(color);
            t4->setColor(color);

            t1->setShine(shine);
            t2->setShine(shine);
            t3->setShine(shine);
            t4->setShine(shine);
            s->setShine(shine);

            objects.push_back(t1);
            objects.push_back(t2);
            objects.push_back(t3);
            objects.push_back(t4);
            objects.push_back(s);
        }
        else
        {
            j = 0;
            normal_light = stoi(line);
            for (int i = 0; i < normal_light; i++)
            {
                for (int k = 0; k < 3; k++)
                {
                    getline(file, line);
                    istringstream iss6(line);
                    while (iss6 >> token)
                    {
                        float number = stod(token);
                        tokens[j] = number;
                        j++;
                    }
                }
                point position(tokens[0], tokens[1], tokens[2]);
                point color(tokens[3], tokens[4], tokens[5]);
                Light nl(position, color, tokens[6]);
                normal_lights.push_back(nl);
            }
            getline(file, line);
            break;
        }
        getline(file, line);
    }
    getline(file, line);
    spot_light = stoi(line);
    j = 0;
    for (int i = 0; i < spot_light; i++)
    {
        for (int k = 0; k < 4; k++)
        {
            getline(file, line);
            // cout << line << endl;
            istringstream iss7(line);
            while (iss7 >> token)
            {
                float number = stod(token);
                // cout << number << endl;
                tokens[j] = number;
                j++;
            }
        }
        point position(tokens[0], tokens[1], tokens[2]);
        point color(tokens[3], tokens[4], tokens[5]);
        Light nl(position, color, tokens[6]);
        point direction(tokens[7]-tokens[0], tokens[8]-tokens[1], tokens[9]-tokens[2]);
        direction.normalize();
        struct SpotLight sl(nl, direction, tokens[10]);
        spot_lights.push_back(sl);
    }
    file.close();

    // cout << "Total objects: " << objects.size() << endl;
    // cout << "Point lights: " << normal_lights.size() << endl;
    // cout << "Spot lights: " << spot_lights.size() << endl;
    // for(int i=0; i<objects.size(); i++){
    //     objects[i]->print();
    // }
    // for (int i = 0; i < spot_lights.size(); i++)
    // {
    //     spot_lights[i].print();
    // }

    // for (int i = 0; i < normal_lights.size(); i++)
    // {
    //     normal_lights[i].print();
    // }
}

/* Main function: GLUT runs as a console application starting at main()  */
int main(int argc, char **argv)
{
    pos.x = 0;
    pos.y = -200;
    pos.z = 35;

    l.x = -pos.x;
    l.y = -pos.y;
    l.z = -pos.z;
    l.normalize();
    point worldUp(0, 0, 1); // Assuming Z is up
    r = l ^ worldUp;
    r.normalize();

    // Calculate the up vector
    u = r ^ l;
    u.normalize();

    readFile();

    glutInit(&argc, argv);                                    // Initialize GLUT
    glutInitWindowSize(768, 768);                             // Set the window's initial width & height
    glutInitWindowPosition(50, 50);                           // Position the window's initial top-left corner
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB); // Depth, Double buffer, RGB color
    glutCreateWindow("OpenGL 3D Drawing");                    // Create a window with the given title
    glutDisplayFunc(display);                                 // Register display callback handler for window re-paint
    glutReshapeFunc(reshapeListener);                         // Register callback handler for window re-shape
    glutKeyboardFunc(keyboardListener);                       // Register callback handler for normal-key event
    glutSpecialFunc(specialKeyListener);                      // Register callback handler for special-key event
    initGL();                                                 // Our own OpenGL initialization
    glutMainLoop();                                           // Enter the event-processing loop
    return 0;
}
