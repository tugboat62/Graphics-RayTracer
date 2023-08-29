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

double determinant(double ara[3][3])
{
    double v1 = ara[0][0] * (ara[1][1] * ara[2][2] - ara[1][2] * ara[2][1]);
    double v2 = ara[0][1] * (ara[1][0] * ara[2][2] - ara[1][2] * ara[2][0]);
    double v3 = ara[0][2] * (ara[1][0] * ara[2][1] - ara[1][1] * ara[2][0]);
    return v1 - v2 + v3;
}

struct line {
    point p1, p2;
};
 
bool onLine(line l1, point p)
{
    // Check whether p is on the line or not
    if (p.x <= max(l1.p1.x, l1.p2.x)
        && p.x >= min(l1.p1.x, l1.p2.x)
        && (p.y <= max(l1.p1.y, l1.p2.y)
            && p.y >= min(l1.p1.y, l1.p2.y)))
        return true;
 
    return false;
}
 
int direction(point a, point b, point c)
{
    int val = (b.y - a.y) * (c.x - b.x)
              - (b.x - a.x) * (c.y - b.y);
 
    if (val == 0)
 
        // Collinear
        return 0;
 
    else if (val < 0)
 
        // Anti-clockwise direction
        return 2;
 
    // Clockwise direction
    return 1;
}
 
bool isIntersect(line l1, line l2)
{
    // Four direction for two lines and points of other line
    int dir1 = direction(l1.p1, l1.p2, l2.p1);
    int dir2 = direction(l1.p1, l1.p2, l2.p2);
    int dir3 = direction(l2.p1, l2.p2, l1.p1);
    int dir4 = direction(l2.p1, l2.p2, l1.p2);
 
    // When intersecting
    if (dir1 != dir2 && dir3 != dir4)
        return true;
 
    // When p2 of line2 are on the line1
    if (dir1 == 0 && onLine(l1, l2.p1))
        return true;
 
    // When p1 of line2 are on the line1
    if (dir2 == 0 && onLine(l1, l2.p2))
        return true;
 
    // When p2 of line1 are on the line2
    if (dir3 == 0 && onLine(l2, l1.p1))
        return true;
 
    // When p1 of line1 are on the line2
    if (dir4 == 0 && onLine(l2, l1.p2))
        return true;
 
    return false;
}
 
bool checkInside(point poly[], int n, point p)
{
 
    // When polygon has less than 3 edge, it is not polygon
    if (n < 3)
        return false;
 
    // Create a point at infinity, y is same as point p
    line exline = { p, { 9999, p.y, 0 } };
    int count = 0;
    int i = 0;
    do {
 
        // Forming a line from two consecutive points of
        // poly
        line side = { poly[i], poly[(i + 1) % n] };
        if (isIntersect(side, exline)) {
 
            // If side is intersects exline
            if (direction(side.p1, p, side.p2) == 0)
                return onLine(side, p);
            count++;
        }
        i = (i + 1) % n;
    } while (i != 0);
 
    // When count is odd
    return count & 1;
}

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
    virtual double intersect(Ray ray, point &col, int level) = 0;
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
        tiles = 50;
        reference_point = point(-(tiles * tilewidth) / 2, -(tiles * tilewidth) / 2, 0);
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
        glBegin(GL_QUADS);
        for (int i = -tiles; i < tiles; i++)
        {
            for (int j = -tiles; j < tiles; j++)
            {
                if ((i + j) % 2 == 0)
                {
                    glColor3f(1, 1, 1);
                }
                else
                {
                    glColor3f(0, 0, 0);
                }
                glVertex3f(i * length, j * length, 0.0f);
                glVertex3f((i + 1) * length, j * length, 0.0f);
                glVertex3f((i + 1) * length, (j + 1) * length, 0.0f);
                glVertex3f(i * length, (j + 1) * length, 0.0f);
            }
        }
        glEnd();
    }

    virtual double intersect(Ray ray, point &col, int level)
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
        col = getColorAt(p);

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

    virtual double intersect(Ray ray, point &col, int level)
    {
        double betaMat[3][3] = {
            {a.x - ray.origin.x, a.x - c.x, ray.dir.x},
            {a.y - ray.origin.y, a.y - c.y, ray.dir.y},
            {a.z - ray.origin.z, a.z - c.z, ray.dir.z}};
        double gammaMat[3][3] = {
            {a.x - b.x, a.x - ray.origin.x, ray.dir.x},
            {a.y - b.y, a.y - ray.origin.y, ray.dir.y},
            {a.z - b.z, a.z - ray.origin.z, ray.dir.z}};
        double tMat[3][3] = {
            {a.x - b.x, a.x - c.x, a.x - ray.origin.x},
            {a.y - b.y, a.y - c.y, a.y - ray.origin.y},
            {a.z - b.z, a.z - c.z, a.z - ray.origin.z}};
        double AMat[3][3]{
            {a.x - b.x, a.x - c.x, ray.dir.x},
            {a.y - b.y, a.y - c.y, ray.dir.y},
            {a.z - b.z, a.z - c.z, ray.dir.z}};

        double Adet = determinant(AMat);
        double beta = determinant(betaMat) / Adet;
        double gamma = determinant(gammaMat) / Adet;
        double t = determinant(tMat) / Adet;

        if (beta + gamma < 1 && beta > 0 && gamma > 0 && t > 0)
        {
            col = color;
            return t;
        }
        else
        {
            return -1;
        }
    }
};

bool isPointInsideSquare(point pt, point a, point b, point c, point d) {
    double minX = std::min(std::min(a.x, b.x), std::min(c.x, d.x));
    double maxX = std::max(std::max(a.x, b.x), std::max(c.x, d.x));
    
    double minY = std::min(std::min(a.y, b.y), std::min(c.y, d.y));
    double maxY = std::max(std::max(a.y, b.y), std::max(c.y, d.y));
    
    double minZ = std::min(std::min(a.z, b.z), std::min(c.z, d.z));
    double maxZ = std::max(std::max(a.z, b.z), std::max(c.z, d.z));
    
    return (pt.x >= minX && pt.x <= maxX &&
            pt.y >= minY && pt.y <= maxY &&
            pt.z >= minZ && pt.z <= maxZ);
}

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

    virtual double intersect(Ray ray, point &col, int level)
    {
        // Find the normal of the square plane
        point normal = (b - a) ^ (c - a);
        normal.normalize();

        // Calculate the dot product of the ray direction and the normal
        double denom = normal * (ray.dir);

        // Check if the ray and the plane are not parallel
        if (std::abs(denom) > 1e-6)
        {
            // Calculate the distance from the ray's origin to the plane
            double t = normal * (a - ray.origin) / denom;

            // Check if the intersection point is within the bounds of the square
            point intersectionPoint = ray.origin + ray.dir * t;
            if (isPointInsideSquare(intersectionPoint, a, b, c, d))
            {
                col = color;
                return t;
            }
        }

        // No intersection or invalid intersection point
        return -1.0;
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
        glColor3f(color.x, color.y, color.z);
        glTranslated(reference_point.x, reference_point.y, reference_point.z);
        glutSolidSphere(length, 50, 50);
        glPopMatrix();
    }

    virtual Ray getNormal(point pt, Ray incidentRay)
    {
        return Ray(pt, pt - reference_point);
    }

    virtual double intersect(Ray ray, point &col, int level)
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
            col = color;
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

