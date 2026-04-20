#include "lodepng.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>
#include <chrono>
#include <corecrt_math_defines.h>

struct Vertex {
    float x, y, z;
    Vertex() : x(0), y(0), z(0) {}
    Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct Normal {
    float x, y, z;
    Normal() : x(0), y(0), z(0) {}
    Normal(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct TexCoord {
    float u, v;
    TexCoord() : u(0), v(0) {}
    TexCoord(float u_, float v_) : u(u_), v(v_) {}
};

struct VertexData {
    Vertex pos;
    Normal normal;
    TexCoord tex;
};

struct BarycentricCoords {
    float lambda0, lambda1, lambda2;
};

/*в файле лекции матрица выглядит громоздко,
можно воспользоваться свойствами кватернионов и привести
матрицу к более приятному виду*/
std::vector<std::vector<float>> quaternionToRotationMatrix(float w, float x, float y, float z) {
    return {
        {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
        {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
        {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}
    };
}

std::vector<float> axisAngleToQuaternion(float axis_x, float axis_y, float axis_z, float angle_degrees) {
    float angle_rad = angle_degrees * M_PI / 180.0f;
    float length = std::sqrt(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
    axis_x /= length;
    axis_y /= length;
    axis_z /= length;

    float sin_half = std::sin(angle_rad / 2);
    float cos_half = std::cos(angle_rad / 2);

    return { cos_half, axis_x * sin_half, axis_y * sin_half, axis_z * sin_half };
}

Vertex rotateWithQuaternion(const Vertex& v, float angle_degrees,
    float axis_x, float axis_y, float axis_z,
    float shift_x, float shift_y, float shift_z) {
    auto q = axisAngleToQuaternion(axis_x, axis_y, axis_z, angle_degrees);
    auto R = quaternionToRotationMatrix(q[0], q[1], q[2], q[3]);

    float x = v.x * R[0][0] + v.y * R[0][1] + v.z * R[0][2];
    float y = v.x * R[1][0] + v.y * R[1][1] + v.z * R[1][2];
    float z = v.x * R[2][0] + v.y * R[2][1] + v.z * R[2][2];

    return Vertex(x + shift_x, y + shift_y, z + shift_z);
}

Vertex rotateEuler(const Vertex& v, float x_angle, float y_angle, float z_angle,
    float shift_x, float shift_y, float shift_z) {
    float x_rad = x_angle * M_PI / 180.0f;
    float y_rad = y_angle * M_PI / 180.0f;
    float z_rad = z_angle * M_PI / 180.0f;

    float cx = std::cos(x_rad), sx = std::sin(x_rad);
    float cy = std::cos(y_rad), sy = std::sin(y_rad);
    float cz = std::cos(z_rad), sz = std::sin(z_rad);

    float x = v.x * (cy * cz) + v.y * (cy * sz) + v.z * (-sy);
    float y = v.x * (sx * sy * cz - cx * sz) + v.y * (sx * sy * sz + cx * cz) + v.z * (sx * cy);
    float z = v.x * (cx * sy * cz + sx * sz) + v.y * (cx * sy * sz - sx * cz) + v.z * (cx * cy);

    return Vertex(x + shift_x, y + shift_y, z + shift_z);
}

Normal triangleNormal(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    float ux = v1.x - v0.x;
    float uy = v1.y - v0.y;
    float uz = v1.z - v0.z;

    float vx = v2.x - v0.x;
    float vy = v2.y - v0.y;
    float vz = v2.z - v0.z;

    Normal n;
    n.x = uy * vz - uz * vy;
    n.y = uz * vx - ux * vz;
    n.z = ux * vy - uy * vx;

    float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (length > 1e-6f) {
        n.x /= length;
        n.y /= length;
        n.z /= length;
    }
    return n;
}

std::vector<Normal> computeVertexNormals(const std::vector<Vertex>& vertices,
    const std::vector<std::vector<int>>& faces) {
    std::vector<Normal> vertexNormals(vertices.size(), Normal(0, 0, 0));
    std::vector<int> vertexCount(vertices.size(), 0);

    for (const auto& face : faces) {
        if (face.size() < 3) continue;

        Normal triNormal = triangleNormal(vertices[face[0]], vertices[face[1]], vertices[face[2]]);

        for (int idx : face) {
            vertexNormals[idx].x += triNormal.x;
            vertexNormals[idx].y += triNormal.y;
            vertexNormals[idx].z += triNormal.z;
            vertexCount[idx]++;
        }
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        if (vertexCount[i] > 0) {
            vertexNormals[i].x /= vertexCount[i];
            vertexNormals[i].y /= vertexCount[i];
            vertexNormals[i].z /= vertexCount[i];

            float length = std::sqrt(vertexNormals[i].x * vertexNormals[i].x +
                vertexNormals[i].y * vertexNormals[i].y +
                vertexNormals[i].z * vertexNormals[i].z);
            if (length > 1e-6f) {
                vertexNormals[i].x /= length;
                vertexNormals[i].y /= length;
                vertexNormals[i].z /= length;
            }
        }
    }

    return vertexNormals;
}

BarycentricCoords barycentricCoordinates(float x0, float y0, float x1, float y1,
    float x2, float y2, float x, float y) {
    BarycentricCoords coords;
    float denominator = (x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2);

    if (std::abs(denominator) < 1e-6f) {
        coords.lambda0 = -1;
        coords.lambda1 = -1;
        coords.lambda2 = -1;
        return coords;
    }

    coords.lambda0 = ((x - x2) * (y1 - y2) - (x1 - x2) * (y - y2)) / denominator;
    coords.lambda1 = ((x0 - x2) * (y - y2) - (x - x2) * (y0 - y2)) / denominator;
    coords.lambda2 = 1.0f - coords.lambda0 - coords.lambda1;

    return coords;
}

void project(float x0, float y0, float z0, float x1, float y1, float z1,
    float x2, float y2, float z2, float scale, float width, float height,
    float& px0, float& py0, float& px1, float& py1, float& px2, float& py2) {
    px0 = scale * x0 / z0 + width / 2;
    py0 = scale * y0 / z0 + height / 2;
    px1 = scale * x1 / z1 + width / 2;
    py1 = scale * y1 / z1 + height / 2;
    px2 = scale * x2 / z2 + width / 2;
    py2 = scale * y2 / z2 + height / 2;
}

/*"лайфхак" из телеграм беседы "Компьютерная графика"
или обычная триангуляция многоугольника*/
void triangulate(const std::vector<int>& poly, std::vector<std::vector<int>>& triangles) {
    for (size_t i = 1; i < poly.size() - 1; i++) {
        triangles.push_back({ poly[0], poly[i], poly[i + 1] });
    }
}

bool loadOBJ(const std::string& filename,
    std::vector<Vertex>& vertices,
    std::vector<TexCoord>& texcoords,
    std::vector<std::vector<int>>& faces,
    std::vector<std::vector<int>>& texFaces) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return false;
    }

    std::vector<Vertex> tempVertices;
    std::vector<TexCoord> tempTexcoords;
    std::vector<std::vector<int>> tempFaces;
    std::vector<std::vector<int>> tempTexFaces;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            tempVertices.push_back(Vertex(x, y, z));
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            tempTexcoords.push_back(TexCoord(u, v));
        }
        else if (type == "f") {
            std::vector<int> faceVerts;
            std::vector<int> faceTex;
            std::string part;

            while (iss >> part) {
                size_t slash1 = part.find('/');
                int v_idx = std::stoi(part.substr(0, slash1)) - 1;
                faceVerts.push_back(v_idx);

                if (slash1 != std::string::npos) {
                    size_t slash2 = part.find('/', slash1 + 1);
                    int vt_idx = std::stoi(part.substr(slash1 + 1, slash2 - slash1 - 1)) - 1;
                    faceTex.push_back(vt_idx);
                }
            }

            if (faceVerts.size() == 3) {
                tempFaces.push_back(faceVerts);
                tempTexFaces.push_back(faceTex);
            }
            else if (faceVerts.size() > 3) {
                triangulate(faceVerts, tempFaces);
                if (!faceTex.empty()) {
                    triangulate(faceTex, tempTexFaces);
                }
            }
        }
    }

    file.close();

    vertices = tempVertices;
    texcoords = tempTexcoords;
    faces = tempFaces;
    texFaces = tempTexFaces;

    std::cout << "Загружено вершин: " << vertices.size()
        << ", полигонов: " << faces.size() << std::endl;
    return true;
}


std::vector<unsigned char> loadTexture(const std::string& filename, int& width, int& height) {
    std::vector<unsigned char> image;
    unsigned int w, h;
    unsigned error = lodepng::decode(image, w, h, filename);

    if (error) {
        std::cerr << "Ошибка загрузки текстуры: " << lodepng_error_text(error) << std::endl;
        width = 0;
        height = 0;
        return {};
    }

    width = w;
    height = h;
    std::cout << "Загружена текстура: " << width << "x" << height << std::endl;
    return image;
}


void drawTriangle(std::vector<unsigned char>& framebuffer, int width, int height,
    const VertexData& v0, const VertexData& v1, const VertexData& v2,
    float scale, std::vector<float>& zbuffer,
    const std::vector<unsigned char>* texture = nullptr,
    int texWidth = 0, int texHeight = 0,
    const unsigned char* color = nullptr) {

    float px0, py0, px1, py1, px2, py2;
    project(v0.pos.x, v0.pos.y, v0.pos.z, v1.pos.x, v1.pos.y, v1.pos.z,
        v2.pos.x, v2.pos.y, v2.pos.z, scale, width, height,
        px0, py0, px1, py1, px2, py2);

    py0 = height - 1 - py0;
    py1 = height - 1 - py1;
    py2 = height - 1 - py2;

    int xmin = std::max(0, (int)std::floor(std::min({ px0, px1, px2 })));
    int xmax = std::min(width - 1, (int)std::ceil(std::max({ px0, px1, px2 })));
    int ymin = std::max(0, (int)std::floor(std::min({ py0, py1, py2 })));
    int ymax = std::min(height - 1, (int)std::ceil(std::max({ py0, py1, py2 })));

    Normal lightDir(0, 0, 1);

    float I0 = std::max(0.0f, v0.normal.x * lightDir.x + v0.normal.y * lightDir.y + v0.normal.z * lightDir.z);
    float I1 = std::max(0.0f, v1.normal.x * lightDir.x + v1.normal.y * lightDir.y + v1.normal.z * lightDir.z);
    float I2 = std::max(0.0f, v2.normal.x * lightDir.x + v2.normal.y * lightDir.y + v2.normal.z * lightDir.z);

    unsigned char defaultColor[3] = { 255, 255, 255 };
    if (color == nullptr) {
        color = defaultColor;
    }

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            BarycentricCoords bc = barycentricCoordinates(px0, py0, px1, py1, px2, py2, x, y);

            if (bc.lambda0 >= 0 && bc.lambda1 >= 0 && bc.lambda2 >= 0) {
                float z = v0.pos.z * bc.lambda0 + v1.pos.z * bc.lambda1 + v2.pos.z * bc.lambda2;
                size_t idx = y * width + x;

                if (z > zbuffer[idx]) {
                    continue;
                }
                zbuffer[idx] = z;

                float I = bc.lambda0 * I0 + bc.lambda1 * I1 + bc.lambda2 * I2;

                unsigned char r, g, b;

                if (texture != nullptr && !texture->empty()) {
                    float u = bc.lambda0 * v0.tex.u + bc.lambda1 * v1.tex.u + bc.lambda2 * v2.tex.u;
                    float v = bc.lambda0 * v0.tex.v + bc.lambda1 * v1.tex.v + bc.lambda2 * v2.tex.v;

                    int tex_x = std::min((int)(u * (texWidth - 1)), texWidth - 1);
                    int tex_y = std::min((int)((1 - v) * (texHeight - 1)), texHeight - 1);

                    size_t texIdx = (tex_y * texWidth + tex_x) * 3;
                    r = (unsigned char)((*texture)[texIdx + 0] * I);
                    g = (unsigned char)((*texture)[texIdx + 1] * I);
                    b = (unsigned char)((*texture)[texIdx + 2] * I);
                }
                else {
                    r = (unsigned char)(color[0] * I);
                    g = (unsigned char)(color[1] * I);
                    b = (unsigned char)(color[2] * I);
                }

                size_t pixelIdx = (y * width + x) * 3;
                framebuffer[pixelIdx + 0] = r;
                framebuffer[pixelIdx + 1] = g;
                framebuffer[pixelIdx + 2] = b;
            }
        }
    }
}

void clearCin() {
    std::cin.clear(); 
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  
}

int main() {
    setlocale(LC_ALL, "Rus");

    std::cout << "Привет! Это программа для рендеринга." << std::endl;

    int width, height;
    float scale;
    std::string outputFile;

    std::cout << "Ширина изображения: ";
    std::cin >> width;
    clearCin();
    std::cout << "Длина изображения: ";
    std::cin >> height;
    clearCin();
    std::cout << "Введите коэффицент масштаба: ";
    std::cin >> scale;
    clearCin();
    std::cout << "Введите имя выходного файла (.png): ";
    std::cin >> outputFile;
    clearCin();
    std::vector<unsigned char> framebuffer(width * height * 3, 255); 
    std::vector<float> zbuffer(width * height, std::numeric_limits<float>::max());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> colorDist(0, 255);

    while (true) {
        std::cout << "\n1. Загрузить и отрисовать" << std::endl;
        std::cout << "2. Завершить отрисовку и сохранить" << std::endl;
        std::cout << "Выберите вариант: ";
        int choice;
        std::cin >> choice;
        clearCin();
        if (choice == 2) {
            break;
        }

        std::string objFile;
        std::cout << "Введите имя .obj файла: ";
        std::cin >> objFile;
        clearCin();
        std::string textureFile;
        std::vector<unsigned char> texture;
        int texWidth = 0, texHeight = 0;
        unsigned char customColor[3];
        bool useTexture = false;

        std::cout << "Нужна ли текстура? (1 - да, 2 - нет): ";
        int texChoice;
        std::cin >> texChoice;
        clearCin();
        if (texChoice == 1) {
            std::cout << "Введите имя файла текстуры: ";
            std::cin >> textureFile;
            clearCin();
            texture = loadTexture(textureFile, texWidth, texHeight);
            if (!texture.empty()) {
                useTexture = true;
            }
        }
        else {
            customColor[0] = colorDist(gen);
            customColor[1] = colorDist(gen);
            customColor[2] = colorDist(gen);
            std::cout << "Сгенерированный цвет: RGB(" << (int)customColor[0] << ","
                << (int)customColor[1] << "," << (int)customColor[2] << ")" << std::endl;
        }

        float xShift, yShift, zShift;
        std::cout << "Сдвиг x: ";
        std::cin >> xShift;
        clearCin();
        std::cout << "Сдвиг y: ";
        std::cin >> yShift;
        clearCin();
        std::cout << "Сдвиг по z: ";
        std::cin >> zShift;
        clearCin();
        std::vector<Vertex> vertices;
        std::vector<TexCoord> texcoords;
        std::vector<std::vector<int>> faces;
        std::vector<std::vector<int>> texFaces;

        if (!loadOBJ(objFile, vertices, texcoords, faces, texFaces)) {
            std::cerr << "Ошибка загрузки модели!" << std::endl;
            continue;
        }

        std::vector<Normal> vertexNormals = computeVertexNormals(vertices, faces);

        std::cout << "Выбери метод вращения: 1 - Углы Эйлера, 2 - Кватернионы: ";
        int rotMethod;
        std::cin >> rotMethod;
        clearCin();
        std::vector<Vertex> transformedVertices = vertices;

        if (rotMethod == 1) {
            float xRot, yRot, zRot;
            std::cout << "Вращение по x в градусах: ";
            std::cin >> xRot;
            clearCin();
            std::cout << "Вращение по y в градусах: ";
            std::cin >> yRot;
            clearCin();
            std::cout << "Вращение по z в градусах: ";
            std::cin >> zRot;
            clearCin();
            for (auto& v : transformedVertices) {
                v = rotateEuler(v, xRot, yRot, zRot, xShift, yShift, zShift);
            }
        }
        else {
            float angle, axisX, axisY, axisZ;
            std::cout << "Угол вращения в градусах: ";
            std::cin >> angle;
            clearCin();
            std::cout << "Ось x: ";
            std::cin >> axisX;
            clearCin();
            std::cout << "Ось y: ";
            std::cin >> axisY;
            clearCin();
            std::cout << "Ось z: ";
            std::cin >> axisZ;
            clearCin();
            for (auto& v : transformedVertices) {
                v = rotateWithQuaternion(v, angle, axisX, axisY, axisZ, xShift, yShift, zShift);
            }
        }

        int renderedFaces = 0;
        for (size_t i = 0; i < faces.size(); i++) {
            const auto& face = faces[i];
            if (face.size() < 3) continue;

            int i0 = face[0], i1 = face[1], i2 = face[2];

            VertexData v0, v1, v2;
            v0.pos = transformedVertices[i0];
            v1.pos = transformedVertices[i1];
            v2.pos = transformedVertices[i2];
            v0.normal = vertexNormals[i0];
            v1.normal = vertexNormals[i1];
            v2.normal = vertexNormals[i2];

            if (i < texFaces.size() && texFaces[i].size() >= 3) {
                int t0 = texFaces[i][0], t1 = texFaces[i][1], t2 = texFaces[i][2];
                if (t0 >= 0 && t0 < (int)texcoords.size()) v0.tex = texcoords[t0];
                if (t1 >= 0 && t1 < (int)texcoords.size()) v1.tex = texcoords[t1];
                if (t2 >= 0 && t2 < (int)texcoords.size()) v2.tex = texcoords[t2];
            }

            Normal faceNormal = triangleNormal(v0.pos, v1.pos, v2.pos);
            if (faceNormal.z > 0) {
                continue;
            }

            if (useTexture && !texture.empty()) {
                drawTriangle(framebuffer, width, height, v0, v1, v2, scale, zbuffer,
                    &texture, texWidth, texHeight);
            }
            else {
                drawTriangle(framebuffer, width, height, v0, v1, v2, scale, zbuffer,
                    nullptr, 0, 0, customColor);
            }

            renderedFaces++;
            if (renderedFaces % 1000 == 0) {
                std::cout << "Отрисовано " << renderedFaces << " полигонов..." << std::endl;
            }
        }

        std::cout << "Отрисовано " << renderedFaces << " треугольников." << std::endl;
    }

    unsigned error = lodepng::encode(outputFile, framebuffer, width, height, LCT_RGB);
    if (error) {
        std::cerr << "Ошибка сохранения: " << lodepng_error_text(error) << std::endl;
    }
    else {
        std::cout << "Сохранено: " << outputFile << std::endl;
    }

    return 0;
}