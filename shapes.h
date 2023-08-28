#include <iostream>
#include <fstream>
#include <iomanip>
#include <GL/glut.h>

using namespace std;

using namespace std;

struct point
{
    double x, y, z;

    point()
    {
        x = y = z = 0.0;
    }

    point(double x, double y, double z) : x(x), y(y), z(z) {}
    point(double x, double y, double z, double n) : x(x), y(y), z(z) {}
    point(const point &p) : x(p.x), y(p.y), z(p.z) {}

    /** arithemtic operations  **/

    point operator+(point b) { return point(x + b.x, y + b.y, z + b.z); }
    point operator-(point b) { return point(x - b.x, y - b.y, z - b.z); }
    point operator*(double b) { return point(x * b, y * b, z * b); }
    point operator/(double b) { return point(x / b, y / b, z / b); }
    double operator*(point b) { return x * b.x + y * b.y + z * b.z; }                                   // DOT PRODUCT
    point operator^(point b) { return point(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); } // CROSS PRODUCT
    point operator-() { return point(-x, -y, -z); }

    double length() { return sqrt(x * x + y * y + z * z); }

    void normalize()
    {
        double len = length();
        x /= len;
        y /= len;
        z /= len;
    }

    /** streams  **/

    friend ostream &operator<<(ostream &out, point p)
    {
        out << "(" << p.x << "," << p.y << "," << p.z << ")";
        return out;
    }

    friend istream &operator>>(istream &in, point &p)
    {
        in >> p.x >> p.y >> p.z;
        return in;
    }

    friend ofstream &operator<<(ofstream &output, point &p)
    {
        output << fixed << setprecision(7) << p.x << " " << p.y << " " << p.z;
        return output;
    }
};

struct Ray
{
    point origin, dir;

    Ray(point origin, point dir)
    {
        this->origin = origin;
        dir.normalize();
        this->dir = dir;
    }

    // stream
    friend ostream &operator<<(ostream &out, Ray r)
    {
        out << "Origin : " << r.origin << ", Direction : " << r.dir;
        return out;
    }
};

struct Light
{
    point pos;
    point color;
    double falloff;

    Light(point pos, point color, double falloff) : pos(pos), color(color), falloff(falloff) {}

    void draw()
    {
        glPointSize(5);
        glBegin(GL_POINTS);
        glColor3f(color.x, color.y, color.z);
        glVertex3f(pos.x, pos.y, pos.z);
        glEnd();
    }
};

// spotlight
struct SpotLight
{
    Light pointLight;
    point dir;
    double cutoffAngle; // this is different from the spotlight

    SpotLight(Light pointLight, point dir, double cutoffAngle) : pointLight(pointLight), dir(dir), cutoffAngle(cutoffAngle) {}

    void draw()
    {
        point color = pointLight.color;
        point pos = pointLight.pos;

        glPointSize(15);
        glBegin(GL_POINTS);
        glColor3f(color.x, color.y, color.z);
        glVertex3f(pos.x, pos.y, pos.z);
        glEnd();
    }
};

class Object
{
public:
    point reference_point;
    double height, width, length;
    point color;
    double kd, ks, ka, kr;
    int shine;
    Object()
    {
        reference_point = point(0, 0, 0);
        height = width = length = 0;
        color = point(0, 0, 0);
        shine = kd = ks = ka = kr = 0;
    }
    virtual void draw() = 0;
    // virtual Ray getNormal(point pt, Ray incidentRay) = 0;
    virtual double intersect(Ray ray, point col, int level) = 0;
    virtual void print()
    {
        cout << "Reference Point: " << reference_point << endl;
        cout << "Height: " << height << endl;
        cout << "Width: " << width << endl;
        cout << "Length: " << length << endl;
        cout << "Color: " << color << endl;
        cout << "Shine: " << shine << endl;
        cout << "kd: " << kd << endl;
        cout << "ks: " << ks << endl;
        cout << "ka: " << ka << endl;
        cout << "kr: " << kr << endl;
        cout << endl;
    }
    void setColor(point c)
    {
        color = c;
    }
    void setShine(int s)
    {
        this->shine = s;
    }
    void setCoEfficients(double kd, double ks, double ka, double kr)
    {
        this->kd = kd;
        this->ks = ks;
        this->ka = ka;
        this->kr = kr;
    }
    void setReferencePoint(point p)
    {
        reference_point = p;
    }
};

struct Floor : public Object
{
    int tiles;
    Floor()
    {
        tiles = 1;
    }

    Floor(double tilewidth)
    {
        tiles = 400 / tilewidth;
        reference_point = point(-400 / 2, -400 / 2, 0);
        length = tilewidth;
    }

    virtual point getColorAt(point pt)
    {

        int tileX = (pt.x - reference_point.x) / length;
        int tileY = (pt.y - reference_point.y) / length;

        if (tileX < 0 || tileX >= tiles || tileY < 0 || tileY >= tiles)
        {
            return point(0, 0, 0);
        }

        if (((tileX + tileY) % 2) == 0)
        {
            return point(1, 1, 1);
        }
        else
        {
            return point(0, 0, 0);
        }
    }

    virtual void draw()
    {
        for (int i = 0; i < tiles; i++)
        {
            for (int j = 0; j < tiles; j++)
            {
                if (((i + j) % 2) == 0)
                    glColor3f(1, 1, 1);
                else
                    glColor3f(0, 0, 0);

                glBegin(GL_QUADS);
                {
                    glVertex3f(reference_point.x + i * length, reference_point.y + j * length, 0);
                    glVertex3f(reference_point.x + (i + 1) * length, reference_point.y + j * length, 0);
                    glVertex3f(reference_point.x + (i + 1) * length, reference_point.y + (j + 1) * length, 0);
                    glVertex3f(reference_point.x + i * length, reference_point.y + (j + 1) * length, 0);
                }
                glEnd();
            }
        }
    }

    virtual double intersect(Ray ray, point &color, int level)
    {
        point normal = point(0, 0, 1);
        double dotP = normal * ray.dir;

        if (round(dotP * 100) == 0)
            return -1;

        double t = -(normal * ray.origin) / dotP;

        point p = ray.origin + ray.dir * t;

        if (p.x <= reference_point.x || p.x >= abs(reference_point.x) && p.y <= reference_point.y && p.y >= abs(reference_point.y))
        {
            return -1;
        }

        return t;
    }
};

struct triangle : public Object
{
    point a, b, c;

    triangle()
    {
    }

    triangle(point a, point b, point c)
    {
        this->a = a;
        this->b = b;
        this->c = c;
    }

    virtual Ray getNormal(point pt, Ray incidentRay)
    {
        point normal = (b - a) ^ (c - a);
        normal.normalize();

        if (incidentRay.dir * normal < 0)
        {
            return Ray(pt, -normal);
        }
        else
        {
            return Ray(pt, normal);
        }
    }

    virtual void draw()
    {
        glColor3f(color.x, color.y, color.z);
        glBegin(GL_TRIANGLES);
        {
            glVertex3f(a.x, a.y, a.z);
            glVertex3f(b.x, b.y, b.z);
            glVertex3f(c.x, c.y, c.z);
        }
        glEnd();
    }

    virtual double intersect(Ray ray, point col, int level)
    {
        point ab = b - a;
        point ac = c - a;
        point pvec = ray.dir ^ ac;
        double det = ab * pvec;
        if (det < 0.0000001)
            return -1;
        double invDet = 1 / det;
        point tvec = ray.origin - a;
        double u = (tvec * pvec) * invDet;
        if (u < 0 || u > 1)
            return -1;
        point qvec = tvec ^ ab;
        double v = (ray.dir * qvec) * invDet;
        if (v < 0 || u + v > 1)
            return -1;
        double t = (ac * qvec) * invDet;
        return t;
    }
};

struct square : public Object
{
    point a, b, c, d;

    square(point a, point b, point c, point d)
    {
        this->a = a;
        this->b = b;
        this->c = c;
        this->d = d;
    }

    virtual void draw()
    {
        glColor3f(color.x, color.y, color.z);
        glBegin(GL_QUADS);
        {
            glVertex3f(a.x, a.y, a.z);
            glVertex3f(b.x, b.y, b.z);
            glVertex3f(c.x, c.y, c.z);
            glVertex3f(d.x, d.y, d.z);
        }
        glEnd();
    }

    virtual Ray getNormal(point pt, Ray incidentRay)
    {
        // Assuming the plane is defined by points a, b, c (counterclockwise order)
        point normal = (b - a) ^ (c - a);
        normal.normalize();

        float d = -normal * a; // Plane equation: ax + by + cz + d = 0

        float t = -(normal * incidentRay.origin + d) / (normal * incidentRay.dir);

        point intersectionPoint = incidentRay.origin + incidentRay.dir * t;

        // Check the dot product of the normal and the ray direction
        // If it's negative, flip the normal to face the ray
        if (normal * incidentRay.dir > 0)
        {
            normal = -normal;
        }

        Ray normalRay(intersectionPoint, normal);
        return normalRay;
    }

    virtual double intersect(Ray ray, point col, int level)
    {
        point ab = b - a;
        point ac = c - a;
        point pvec = ray.dir ^ ac;
        double det = ab * pvec;
        if (det < 0.0000001)
            return -1;
        double invDet = 1 / det;
        point tvec = ray.origin - a;
        double u = (tvec * pvec) * invDet;
        if (u < 0 || u > 1)
            return -1;
        point qvec = tvec ^ ab;
        double v = (ray.dir * qvec) * invDet;
        if (v < 0 || u + v > 1)
            return -1;
        double t = (ac * qvec) * invDet;
        return t;
    }
};

struct sphere : public Object
{
    sphere()
    {
    }

    sphere(point center, double radius)
    {
        reference_point = center;
        length = width = height = radius;
    }

    virtual void draw()
    {
        glPushMatrix();
        glTranslated(reference_point.x, reference_point.y, reference_point.z);
        glutSolidSphere(length, 50, 50);
        glPopMatrix();
    }

    virtual Ray getNormal(point pt, Ray incidentRay)
    {
        return Ray(pt, pt - reference_point);
    }

    virtual double intersect(Ray ray, point col, int level)
    {
        point oc = ray.origin - reference_point;
        double a = ray.dir * ray.dir;
        double b = 2 * (oc * ray.dir);
        double c = (oc * oc) - length * length;
        double d = b * b - 4 * a * c;
        if (d < 0)
            return -1;
        else
        {
            double t1 = (-b + sqrt(d)) / (2 * a);
            double t2 = (-b - sqrt(d)) / (2 * a);
            if (t1 < 0 && t2 < 0)
                return -1;
            else if (t1 < 0)
                return t2;
            else if (t2 < 0)
                return t1;
            else
                return min(t1, t2);
        }
    }
};