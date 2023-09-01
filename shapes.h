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

    void print()
    {
        cout << "Point Light: " << endl;
        cout << "Position: " << pos << endl;
        cout << "Color: " << color << endl;
        cout << "Falloff: " << falloff << endl;
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
        glColor3f(color.x, color.y, color.z);
        glBegin(GL_POINTS);
        glVertex3f(pos.x, pos.y, pos.z);
        glEnd();
    }

    void print()
    {
        cout << "Spot Light: " << endl;
        cout << "Position: " << pointLight.pos << endl;
        cout << "Color: " << pointLight.color << endl;
        cout << "Falloff: " << pointLight.falloff << endl;
        cout << "Direction: " << dir << endl;
        cout << "Cutoff Angle: " << cutoffAngle << endl;
    }
};

double determinant(double ara[3][3])
{
    double v1 = ara[0][0] * (ara[1][1] * ara[2][2] - ara[1][2] * ara[2][1]);
    double v2 = ara[0][1] * (ara[1][0] * ara[2][2] - ara[1][2] * ara[2][0]);
    double v3 = ara[0][2] * (ara[1][0] * ara[2][1] - ara[1][1] * ara[2][0]);
    return v1 - v2 + v3;
}
class Object;

extern vector<Light> normal_lights;
extern vector<SpotLight> spot_lights;
extern vector<Object *> objects;
extern int recursion_level;

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
    virtual double intersect_shapes(Ray ray, point &col) = 0;
    virtual point getColorAt(point pt)
    {
        return color;
    }
    virtual Ray getNormal(point pt, Ray incidentRay) = 0;
    virtual double intersect(Ray ray, point &col, int level)
    {
        double t = intersect_shapes(ray, col);
        if (t < 0)
            return -1;

        point intersection_point = ray.origin + ray.dir * t;

        if (level == 0)
        {
            col = getColorAt(intersection_point);
            return t;
        }

        point color_intersection = getColorAt(intersection_point);

        // cout << "color_intersection: " << color_intersection << endl;
        // Update color with ambience
        col.x = color_intersection.x * ka;
        col.y = color_intersection.y * ka;
        col.z = color_intersection.z * ka;

        double lambert = 0.0, phong = 0.0;

        for (int i = 0; i < normal_lights.size(); i++)
        {
            point position = normal_lights[i].pos;
            point direction = intersection_point - position;
            direction.normalize();

            Ray normal_lightray(position, direction);
            Ray normal = getNormal(intersection_point, normal_lightray);

            bool isShadow = false;
            double dist = (position - intersection_point).length();
            if (dist < 1e-5)
                continue;
            for (int j = 0; j < objects.size(); j++)
            {
                double t2 = objects[j]->intersect_shapes(normal_lightray, col);
                if (t2 > 0 && t2 + 1e-5 < dist)
                {
                    isShadow = true;
                    break;
                }
            }

            if (isShadow)
                continue;
            point toSource = normal_lightray.origin - intersection_point;
            toSource.normalize();
            double scaling_factor = exp(-dist * dist * normal_lights[i].falloff);
            lambert += (max(0.0, toSource * normal.dir)) * scaling_factor;

            double dotProduct = max(0.0, ray.dir * normal.dir);
            point reflection_dir = ray.dir - normal.dir * (2.0 * dotProduct);
            reflection_dir.normalize();
            Ray reflected_ray(intersection_point, reflection_dir);
            phong += pow(max(0.0, reflection_dir * toSource), shine) * scaling_factor;
            // cout << "lambert: " << lambert << endl;

            col.x += kd * lambert * normal_lights[i].color.x;
            col.y += kd * lambert * normal_lights[i].color.y;
            col.z += kd * lambert * normal_lights[i].color.z;
            if (ks > 0)
            {
                col.x += ks * phong * normal_lights[i].color.x;
                col.y += ks * phong * normal_lights[i].color.y;
                col.z += ks * phong * normal_lights[i].color.z;
            }
        }

        // Problem in this section of code. Need to fix it
        for (int i = 0; i < spot_lights.size(); i++)
        {
            point position = spot_lights[i].pointLight.pos;
            point direction = intersection_point - position;
            direction.normalize();

            double dot = direction * spot_lights[i].dir;
            double angle = acos(dot / (direction.length() * spot_lights[i].dir.length())) * (180.0 / M_PI);
            // cout << "angle: " << angle << endl;

            if (fabs(angle) < spot_lights[i].cutoffAngle)
            {
                Ray spot_lightray(position, direction);
                Ray normal = getNormal(intersection_point, spot_lightray);

                bool isShadow = false;
                double dist = (intersection_point - position).length();
                if (dist < 1e-5)
                {
                    // cout << " l: " << lambert << " p: " << phong << endl;
                    continue;
                }
                for (int j = 0; j < objects.size(); j++)
                {
                    double t2 = objects[j]->intersect_shapes(spot_lightray, col);
                    // cout << " l: " << lambert << " p: " << phong << endl;
                    if (t2 > 0 && t2 + 1e-5 < dist)
                    {
                        isShadow = true;
                        break;
                    }
                }

                if (isShadow)
                    continue;
                point toSource = -spot_lightray.dir;
                double scaling_factor = exp(-dist * dist * spot_lights[i].pointLight.falloff);
                lambert += (max(0.0, toSource * normal.dir)) * scaling_factor;

                double dotProduct = max(0.0, ray.dir * normal.dir);
                point reflection_dir = ray.dir - normal.dir * (2.0 * dotProduct);
                reflection_dir.normalize();
                Ray reflected_ray(intersection_point, reflection_dir);
                phong += pow(max(0.0, reflection_dir * toSource), shine) * scaling_factor;
                // cout << " l: " << lambert << " p: " << phong << endl;

                col.x += kd * lambert * spot_lights[i].pointLight.color.x;
                col.y += kd * lambert * spot_lights[i].pointLight.color.y;
                col.z += kd * lambert * spot_lights[i].pointLight.color.z;
                if (ks > 0)
                {
                    col.x += ks * phong * spot_lights[i].pointLight.color.x;
                    col.y += ks * phong * spot_lights[i].pointLight.color.y;
                    col.z += ks * phong * spot_lights[i].pointLight.color.z;
                }
            }
        }

        // I think this part is ok
        if (level <= recursion_level)
        {
            // cout << "level: " << level << endl;
            Ray normal = getNormal(intersection_point, ray);
            double dotProduct = ray.dir * normal.dir;
            point reflection_dir = ray.dir - normal.dir * (2.0 * dotProduct);
            reflection_dir.normalize();

            Ray reflected_ray(intersection_point, reflection_dir);
            reflected_ray.origin = reflected_ray.origin + reflected_ray.dir * 1e-5;

            double t = -1;
            int nearest = -1;

            for (int i = 0; i < objects.size(); i++)
            {
                double t2 = objects[i]->intersect_shapes(reflected_ray, col);
                if (t2 > 0 && (t == -1 || t2 < t))
                {
                    t = t2;
                    nearest = i;
                }
            }

            if (nearest != -1)
            {
                point reflected_color;
                double t = objects[nearest]->intersect(reflected_ray, reflected_color, level + 1);
                col.x += kr * reflected_color.x;
                col.y += kr * reflected_color.y;
                col.z += kr * reflected_color.z;
            }
        }
        return t;
    }
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
    void setCoEfficients(double ka, double kd, double ks, double kr)
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

    virtual Ray getNormal(point pt, Ray incidentRay)
    {
        point dir(0, 0, 1);
        double mul = dir * incidentRay.dir;
        if (mul > 0)
            dir = -dir;
        return {pt, dir};
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

    virtual double intersect_shapes(Ray ray, point &col)
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
        // col = getColorAt(p);

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

    virtual double intersect_shapes(Ray ray, point &col)
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
            // col = color;
            return t;
        }
        else
        {
            return -1;
        }
    }
};

bool isPointInsideSquare(point pt, point a, point b, point c, point d)
{
    double minX = std::min(std::min(a.x, b.x), std::min(c.x, d.x)) - 1e-5;
    double maxX = std::max(std::max(a.x, b.x), std::max(c.x, d.x)) + 1e-5;

    double minY = std::min(std::min(a.y, b.y), std::min(c.y, d.y)) - 1e-5;
    double maxY = std::max(std::max(a.y, b.y), std::max(c.y, d.y)) + 1e-5;

    double minZ = std::min(std::min(a.z, b.z), std::min(c.z, d.z)) - 1e-5;
    double maxZ = std::max(std::max(a.z, b.z), std::max(c.z, d.z)) + 1e-5;

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

    virtual Ray getNormal(point pt, Ray incidentRay)
    {
        point normal = (b - a) ^ (c - a);
        normal.normalize();

        double mul = normal * incidentRay.dir;
        if (mul > 0)
            normal = -normal;
        return {pt, normal};
    }

    virtual double intersect_shapes(Ray ray, point &col)
    {
        // Find the normal of the square plane
        point normal = (b - a) ^ (c - a);
        normal.normalize();

        // Calculate the dot product of the ray direction and the normal
        double denom = normal * (ray.dir);

        // Check if the ray and the plane are not parallel
        if (std::fabs(denom) > 1e-6)
        {
            // Calculate the distance from the ray's origin to the plane
            double t = normal * (a - ray.origin) / denom;

            // Check if the intersection point is within the bounds of the square
            point intersectionPoint = ray.origin + ray.dir * t;
            if (isPointInsideSquare(intersectionPoint, a, b, c, d))
            {
                // col = color;
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
        point dir = pt - reference_point;
        dir.normalize();
        return Ray(pt, dir);
    }

    virtual double intersect_shapes(Ray ray, point &col)
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
            // col = color;
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
