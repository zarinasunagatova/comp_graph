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
#include <corecrt_math_defines.h>

struct Vertex {
    float x, y, z;
    Vertex() : x(0), y(0), z(0) {}
    Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

Vertex rotate_y(const Vertex& v, float angle_rad) {
    float cos_a = cos(angle_rad);
    float sin_a = sin(angle_rad);
    return Vertex(
        v.x * cos_a + v.z * sin_a,
        v.y,
        -v.x * sin_a + v.z * cos_a
    );
}

Vertex rotate_full(const Vertex& v, float alpha, float beta, float gamma) {
    float ca = cos(alpha), sa = sin(alpha);
    float cb = cos(beta), sb = sin(beta);
    float cg = cos(gamma), sg = sin(gamma);

    float x1 = v.x * cg - v.y * sg;
    float y1 = v.x * sg + v.y * cg;
    float z1 = v.z;

    float x2 = x1 * cb + z1 * sb;
    float y2 = y1;
    float z2 = -x1 * sb + z1 * cb;

    float x3 = x2;
    float y3 = y2 * ca - z2 * sa;
    float z3 = y2 * sa + z2 * ca;

    return Vertex(x3, y3, z3);
}

struct BarycentricCoords {
    float lambda0, lambda1, lambda2;
};

BarycentricCoords compute_barycentric(int x, int y,
    float x0, float y0,
    float x1, float y1,
    float x2, float y2) {

    BarycentricCoords coords;
    float denominator = (x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2);
    if (std::abs(denominator) < 1e-6f) {
        coords.lambda0 = -1.0f;
        coords.lambda1 = -1.0f;
        coords.lambda2 = -1.0f;
        return coords;
    }
    coords.lambda0 = ((x - x2) * (y1 - y2) - (x1 - x2) * (y - y2)) / denominator;
    coords.lambda1 = ((x0 - x2) * (y - y2) - (x - x2) * (y0 - y2)) / denominator;
    coords.lambda2 = 1.0f - coords.lambda0 - coords.lambda1;

    return coords;
}

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

struct TexturedVertex {
    float x, y, z;
    float u, v;
    TexturedVertex() : x(0), y(0), z(0), u(0), v(0) {}
    TexturedVertex(float x_, float y_, float z_, float u_, float v_)
        : x(x_), y(y_), z(z_), u(u_), v(v_) {
    }
};

bool load_obj(const std::string& filename,
    std::vector<Vertex>& vertices,
    std::vector<std::vector<int>>& polygons) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Vertex(x, y, z));
        }
        else if (type == "f") {
            std::vector<int> polygon;
            std::string part;
            while (iss >> part) {
                size_t slash_pos = part.find('/');
                int v_idx = std::stoi(part.substr(0, slash_pos)) - 1;
                polygon.push_back(v_idx);
            }
            polygons.push_back(polygon);
        }
    }
    file.close();
    std::cout << "Загружено вершин: " << vertices.size()
        << ", полигонов: " << polygons.size() << std::endl;
    return true;
}

bool load_obj_with_texture(const std::string& filename,
    std::vector<TexturedVertex>& vertices,
    std::vector<std::vector<int>>& polygons) {

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::vector<Vertex> temp_vertices;
    std::vector<TexCoord> temp_texcoords;
    std::vector<Normal> temp_normals;

    std::vector<int> v_indices, vt_indices, vn_indices;
    std::vector<std::vector<int>> face_vt_indices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            temp_vertices.push_back(Vertex(x, y, z));
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            temp_texcoords.push_back(TexCoord(u, v));
        }
        else if (type == "f") {
            std::string part;
            std::vector<int> face_v, face_vt, face_vn;
            while (iss >> part) {
                size_t slash1 = part.find('/');
                size_t slash2 = part.find('/', slash1 + 1);

                int v_idx = std::stoi(part.substr(0, slash1)) - 1;
                face_v.push_back(v_idx);

                if (slash1 != std::string::npos) {
                    int vt_idx = std::stoi(part.substr(slash1 + 1, slash2 - slash1 - 1)) - 1;
                    face_vt.push_back(vt_idx);
                }
            }

            for (size_t j = 0; j < face_v.size(); j++) {
                TexturedVertex tv;
                tv.x = temp_vertices[face_v[j]].x;
                tv.y = temp_vertices[face_v[j]].y;
                tv.z = temp_vertices[face_v[j]].z;

                if (j < face_vt.size() && face_vt[j] >= 0) {
                    tv.u = temp_texcoords[face_vt[j]].u;
                    tv.v = temp_texcoords[face_vt[j]].v;
                }
                else {
                    tv.u = 0; tv.v = 0;
                }

                vertices.push_back(tv);
                v_indices.push_back(vertices.size() - 1);
            }

            polygons.push_back(v_indices);
            v_indices.clear();
        }
    }

    file.close();
    return true;
}

std::vector<unsigned char> load_texture(const std::string& filename, unsigned int& width, unsigned int& height) {
    std::vector<unsigned char> image;
    unsigned error = lodepng::decode(image, width, height, filename);
    if (error) {
        std::cerr << "Ошибка загрузки текстуры: " << lodepng_error_text(error) << std::endl;
        return {};
    }
    std::cout << "Загружена текстура: " << width << "x" << height << std::endl;
    return image;
}

Normal compute_normal(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    float ux = v1.x - v0.x;
    float uy = v1.y - v0.y;
    float uz = v1.z - v0.z;

    float vx = v2.x - v0.x;
    float vy = v2.y - v0.y;
    float vz = v2.z - v0.z;

    Normal normal;
    normal.x = uy * vz - uz * vy;
    normal.y = uz * vx - ux * vz;
    normal.z = ux * vy - uy * vx;

    return normal;
}

Normal normalize(const Normal& n) {
    float length = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (length > 1e-6f) {
        return Normal(n.x / length, n.y / length, n.z / length);
    }
    return Normal(0, 0, 0);
}

std::vector<Normal> compute_vertex_normals(
    const std::vector<Vertex>& vertices,
    const std::vector<std::vector<int>>& polygons) {

    std::vector<Normal> vertex_normals(vertices.size(), Normal(0, 0, 0));
    std::vector<int> vertex_count(vertices.size(), 0);

    for (const auto& poly : polygons) {
        if (poly.size() != 3) continue;

        Normal tri_normal = compute_normal(
            vertices[poly[0]], vertices[poly[1]], vertices[poly[2]]);

        for (int idx : poly) {
            vertex_normals[idx].x += tri_normal.x;
            vertex_normals[idx].y += tri_normal.y;
            vertex_normals[idx].z += tri_normal.z;
            vertex_count[idx]++;
        }
    }

    for (size_t i = 0; i < vertices.size(); i++) {
        if (vertex_count[i] > 0) {
            vertex_normals[i].x /= vertex_count[i];
            vertex_normals[i].y /= vertex_count[i];
            vertex_normals[i].z /= vertex_count[i];
            vertex_normals[i] = normalize(vertex_normals[i]);
        }
    }

    return vertex_normals;
}

float compute_vertex_intensity(const Normal& vertex_normal, const Normal& light_dir) {
    Normal vn = normalize(vertex_normal);
    Normal ld = normalize(light_dir);

    float dot = vn.x * ld.x + vn.y * ld.y + vn.z * ld.z;
    return (std::max)(0.0f, dot);
}

void draw_triangle_gouraud(
    std::vector<unsigned char>& img, int w, int h,
    float x0, float y0, float x1, float y1, float x2, float y2,
    float i0, float i1, float i2,
    std::vector<float>* zbuffer = nullptr,
    float z0 = 0, float z1 = 0, float z2 = 0) {

    int xmin = (int)std::floor((std::min)({ x0, x1, x2 }));
    int xmax = (int)std::ceil((std::max)({ x0, x1, x2 }));
    int ymin = (int)std::floor((std::min)({ y0, y1, y2 }));
    int ymax = (int)std::ceil((std::max)({ y0, y1, y2 }));

    xmin = (std::max)(0, xmin);
    xmax = (std::min)(w - 1, xmax);
    ymin = (std::max)(0, ymin);
    ymax = (std::min)(h - 1, ymax);

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            BarycentricCoords coords = compute_barycentric(x, y, x0, y0, x1, y1, x2, y2);

            if (coords.lambda0 >= 0 && coords.lambda1 >= 0 && coords.lambda2 >= 0) {
                if (zbuffer != nullptr) {
                    float z = coords.lambda0 * z0 + coords.lambda1 * z1 + coords.lambda2 * z2;
                    size_t idx = y * w + x;

                    if (z > (*zbuffer)[idx]) {
                        continue;
                    }
                    (*zbuffer)[idx] = z;
                }

                float intensity = coords.lambda0 * i0 + coords.lambda1 * i1 + coords.lambda2 * i2;
                intensity = (std::max)(0.0f, (std::min)(1.0f, intensity));

                int color_value = (int)(255.0f * intensity);
                unsigned char color = (unsigned char)color_value;

                size_t idx = (y * w + x) * 3;
                img[idx + 0] = color;
                img[idx + 1] = color;
                img[idx + 2] = color;
            }
        }
    }
}

void draw_triangle_textured_with_lighting(
    std::vector<unsigned char>& img, int w, int h,
    float x0, float y0, float x1, float y1, float x2, float y2,
    float u0, float v0, float u1, float v1, float u2, float v2,
    float i0, float i1, float i2,
    const std::vector<unsigned char>& texture, int tex_w, int tex_h,
    std::vector<float>* zbuffer = nullptr,
    float z0 = 0, float z1 = 0, float z2 = 0) {

    int xmin = (int)std::floor((std::min)({ x0, x1, x2 }));
    int xmax = (int)std::ceil((std::max)({ x0, x1, x2 }));
    int ymin = (int)std::floor((std::min)({ y0, y1, y2 }));
    int ymax = (int)std::ceil((std::max)({ y0, y1, y2 }));

    xmin = (std::max)(0, xmin);
    xmax = (std::min)(w - 1, xmax);
    ymin = (std::max)(0, ymin);
    ymax = (std::min)(h - 1, ymax);

    for (int y = ymin; y <= ymax; y++) {
        for (int x = xmin; x <= xmax; x++) {
            BarycentricCoords coords = compute_barycentric(x, y, x0, y0, x1, y1, x2, y2);

            if (coords.lambda0 >= 0 && coords.lambda1 >= 0 && coords.lambda2 >= 0) {
                float z = coords.lambda0 * z0 + coords.lambda1 * z1 + coords.lambda2 * z2;
                size_t idx = y * w + x;

                if (zbuffer != nullptr) {
                    if (z > (*zbuffer)[idx]) {
                        continue;
                    }
                    (*zbuffer)[idx] = z;
                }

                float u = coords.lambda0 * u0 + coords.lambda1 * u1 + coords.lambda2 * u2;
                float v = coords.lambda0 * v0 + coords.lambda1 * v1 + coords.lambda2 * v2;

                u = (std::max)(0.0f, (std::min)(1.0f, u));
                v = (std::max)(0.0f, (std::min)(1.0f, v));

                int tex_x = (int)(u * (tex_w - 1));
                int tex_y = (int)((1.0f - v) * (tex_h - 1));
                tex_x = (std::max)(0, (std::min)(tex_w - 1, tex_x));
                tex_y = (std::max)(0, (std::min)(tex_h - 1, tex_y));

                float intensity = coords.lambda0 * i0 + coords.lambda1 * i1 + coords.lambda2 * i2;
                intensity = (std::max)(0.0f, (std::min)(1.0f, intensity));

                size_t tex_idx = (tex_y * tex_w + tex_x) * 3;
                size_t img_idx = (y * w + x) * 3;

                if (tex_idx + 2 < texture.size()) {
                    img[img_idx + 0] = (unsigned char)(texture[tex_idx + 0] * intensity);
                    img[img_idx + 1] = (unsigned char)(texture[tex_idx + 1] * intensity);
                    img[img_idx + 2] = (unsigned char)(texture[tex_idx + 2] * intensity);
                }
            }
        }
    }
}
/*здесь ортографическая проекция как в 15 задании
не деленим на z
объект не меняет размер с расстоянием
параллельные линии остаются параллельными
классика для cad*/
void task17_gouraud(
    const std::vector<Vertex>& vertices,
    const std::vector<std::vector<int>>& polygons,
    int img_size = 1200) {

    if (vertices.empty() || polygons.empty()) {
        std::cerr << "Нет вершин или полигонов!" << std::endl;
        return;
    }

    float offset_x = img_size / 2.0f;
    float offset_y = img_size / 2.0f;
    float scale = 500.0f;

    float alpha = 0.0f;
    float beta = 180.0f * M_PI / 180.0f;
    float gamma = 0.0f;

    float min_x = vertices[0].x, max_x = vertices[0].x;
    float min_y = vertices[0].y, max_y = vertices[0].y;
    float min_z = vertices[0].z, max_z = vertices[0].z;

    for (const auto& v : vertices) {
        min_x = (std::min)(min_x, v.x);
        max_x = (std::max)(max_x, v.x);
        min_y = (std::min)(min_y, v.y);
        max_y = (std::max)(max_y, v.y);
        min_z = (std::min)(min_z, v.z);
        max_z = (std::max)(max_z, v.z);
    }

    float center_x = (min_x + max_x) / 2.0f;
    float center_y = (min_y + max_y) / 2.0f;
    float center_z = (min_z + max_z) / 2.0f;

    float tX = -center_x;
    float tY = -center_y;
    float tZ = -center_z;

    Normal light_dir(0, 0, 1);
    light_dir = normalize(light_dir);

    std::cout << "Вычисление нормалей вершин..." << std::endl;
    std::vector<Normal> vertex_normals = compute_vertex_normals(vertices, polygons);

    std::vector<float> vertex_intensities(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        vertex_intensities[i] = compute_vertex_intensity(vertex_normals[i], light_dir);
    }

    std::vector<float> zbuffer(img_size * img_size, (std::numeric_limits<float>::max)());
    std::vector<unsigned char> img(img_size * img_size * 3, 0);

    int triangle_count = 0;

    for (const auto& poly : polygons) {
        if (poly.size() != 3) continue;

        Vertex v0 = vertices[poly[0]];
        Vertex v1 = vertices[poly[1]];
        Vertex v2 = vertices[poly[2]];

        v0.x *= scale; v0.y *= scale; v0.z *= scale;
        v1.x *= scale; v1.y *= scale; v1.z *= scale;
        v2.x *= scale; v2.y *= scale; v2.z *= scale;

        v0 = rotate_full(v0, alpha, beta, gamma);
        v1 = rotate_full(v1, alpha, beta, gamma);
        v2 = rotate_full(v2, alpha, beta, gamma);

        v0.x += tX; v0.y += tY; v0.z += tZ;
        v1.x += tX; v1.y += tY; v1.z += tZ;
        v2.x += tX; v2.y += tY; v2.z += tZ;

        float x0 = v0.x + offset_x;
        float y0 = img_size - 1 - (v0.y + offset_y);
        float x1 = v1.x + offset_x;
        float y1 = img_size - 1 - (v1.y + offset_y);
        float x2 = v2.x + offset_x;
        float y2 = img_size - 1 - (v2.y + offset_y);

        float z0 = v0.z;
        float z1 = v1.z;
        float z2 = v2.z;

        float i0 = vertex_intensities[poly[0]];
        float i1 = vertex_intensities[poly[1]];
        float i2 = vertex_intensities[poly[2]];

        draw_triangle_gouraud(img, img_size, img_size,
            x0, y0, x1, y1, x2, y2,
            i0, i1, i2, &zbuffer, z0, z1, z2);

        triangle_count++;
        if (triangle_count % 5000 == 0) {
            std::cout << "Отрисовано " << triangle_count << " треугольников..." << std::endl;
        }
    }

    std::cout << "Задание 17 (Гуро): отрисовано " << triangle_count << " треугольников." << std::endl;
    lodepng::encode("task17_gouraud.png", img, img_size, img_size, LCT_RGB);
    std::cout << "Сохранено: task17_gouraud.png" << std::endl;
}
/*к сожалению, все мои попытки написать функции для текстурирования модели оказались
неуспешны, единственное предположение- 
проблема именно в перспективных искажениях, но и попытки поправить это
ни к чему не привели...*/
/*здесь перспективная проекция как в 16 задании
есть деление на z (глубину)
объекты уменьшаются с удалением
реалистичный "эффект камеры"*/
void task18_textured_with_lighting(

    const std::vector<TexturedVertex>& vertices,
    const std::vector<std::vector<int>>& polygons,
    const std::vector<unsigned char>& texture,
    int tex_width, int tex_height,
    int img_size = 1200) {
    std::cout << "Количество вершин: " << vertices.size() << std::endl;
    std::cout << "Количество полигонов: " << polygons.size() << std::endl;
    if (vertices.empty() || polygons.empty()) {
        std::cerr << "Нет вершин или полигонов!" << std::endl;
        return;
    }
    float alpha = 0.0f;
    float beta = 180.0f * M_PI / 180.0f;
    float gamma = 0.0f;

    float min_x = vertices[0].x, max_x = vertices[0].x;
    float min_y = vertices[0].y, max_y = vertices[0].y;
    float min_z = vertices[0].z, max_z = vertices[0].z;

    for (const auto& v : vertices) {
        min_x = (std::min)(min_x, v.x);
        max_x = (std::max)(max_x, v.x);
        min_y = (std::min)(min_y, v.y);
        max_y = (std::max)(max_y, v.y);
        min_z = (std::min)(min_z, v.z);
        max_z = (std::max)(max_z, v.z);
    }

    float center_x = (min_x + max_x) / 2.0f;
    float center_y = (min_y + max_y) / 2.0f;
    float center_z = (min_z + max_z) / 2.0f;

    float tX = -center_x;
    float tY = -center_y;
    float tZ = -center_z;

    Normal light_dir(0, 0, 1);
    light_dir = normalize(light_dir);

    std::vector<Vertex> simple_vertices;
    for (const auto& v : vertices) {
        simple_vertices.push_back(Vertex(v.x, v.y, v.z));
    }
    std::vector<Normal> vertex_normals = compute_vertex_normals(simple_vertices, polygons);

    std::vector<float> vertex_intensities(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++) {
        vertex_intensities[i] = compute_vertex_intensity(vertex_normals[i], light_dir);
    }

    std::vector<float> zbuffer(img_size * img_size, (std::numeric_limits<float>::max)());
    std::vector<unsigned char> img(img_size * img_size * 3, 0);

    int triangle_count = 0;

    for (size_t i = 0; i < polygons.size(); i++) {
        const auto& poly = polygons[i];
        if (poly.size() != 3) continue;

        TexturedVertex v0 = vertices[poly[0]];
        TexturedVertex v1 = vertices[poly[1]];
        TexturedVertex v2 = vertices[poly[2]];

        Vertex v0_rot = rotate_full(Vertex(v0.x, v0.y, v0.z), alpha, beta, gamma);
        Vertex v1_rot = rotate_full(Vertex(v1.x, v1.y, v1.z), alpha, beta, gamma);
        Vertex v2_rot = rotate_full(Vertex(v2.x, v2.y, v2.z), alpha, beta, gamma);

        v0.x = v0_rot.x; v0.y = v0_rot.y; v0.z = v0_rot.z;
        v1.x = v1_rot.x; v1.y = v1_rot.y; v1.z = v1_rot.z;
        v2.x = v2_rot.x; v2.y = v2_rot.y; v2.z = v2_rot.z;

        v0.x += tX; v0.y += tY; v0.z += tZ;
        v1.x += tX; v1.y += tY; v1.z += tZ;
        v2.x += tX; v2.y += tY; v2.z += tZ;

        float z_offset = 4.0f;
        float aX = 1000.0f;
        float aY = 1000.0f;
        float offset_x = img_size / 2.0f;
        float offset_y = img_size / 2.0f;

        float z0 = v0.z + z_offset;
        float z1 = v1.z + z_offset;
        float z2 = v2.z + z_offset;

        if (z0 <= 0.001f || z1 <= 0.001f || z2 <= 0.001f) continue;

        float x0_proj = aX * v0.x / z0 + offset_x;
        float y0_proj = aY * v0.y / z0 + offset_y;
        float x1_proj = aX * v1.x / z1 + offset_x;
        float y1_proj = aY * v1.y / z1 + offset_y;
        float x2_proj = aX * v2.x / z2 + offset_x;
        float y2_proj = aY * v2.y / z2 + offset_y;

        y0_proj = img_size - 1 - y0_proj;
        y1_proj = img_size - 1 - y1_proj;
        y2_proj = img_size - 1 - y2_proj;

        float i0 = vertex_intensities[poly[0]];
        float i1 = vertex_intensities[poly[1]];
        float i2 = vertex_intensities[poly[2]];

        draw_triangle_textured_with_lighting(img, img_size, img_size,
            x0_proj, y0_proj, x1_proj, y1_proj, x2_proj, y2_proj,
            v0.u, v0.v, v1.u, v1.v, v2.u, v2.v,
            i0, i1, i2, texture, tex_width, tex_height, &zbuffer, z0, z1, z2);
        if (triangle_count == 0 && polygons.size() > 0) {
            std::cout << "Первые UVs: ("
                << vertices[polygons[0][0]].u << "," << vertices[polygons[0][0]].v << "), "
                << vertices[polygons[0][1]].u << "," << vertices[polygons[0][1]].v << "), "
                << vertices[polygons[0][2]].u << "," << vertices[polygons[0][2]].v << ")"
                << std::endl;
        }
        triangle_count++;
        if (triangle_count % 5000 == 0) {
            std::cout << "Нарисовано " << triangle_count << " треугольников..." << std::endl;
        }
    }


    std::cout << "Задание 18: нарисовано " << triangle_count << " треугольников." << std::endl;
    lodepng::encode("task18_textured.png", img, img_size, img_size, LCT_RGB);
    std::cout << "Сохранено: task18_textured.png" << std::endl;
}
int main() {
    setlocale(LC_ALL, "Rus");
    std::string obj_filename = "model.obj";

    std::vector<Vertex> vertices;
    std::vector<std::vector<int>> polygons;

    std::cout << "ЗАГРУЗКА МОДЕЛИ" << std::endl;
    if (!load_obj(obj_filename, vertices, polygons)) {
        std::cerr << "Ошибка загрузки модели!" << std::endl;
        return 1;
    }

    std::cout << "Загружено вершин: " << vertices.size() << std::endl;
    std::cout << "Загружено полигонов: " << polygons.size() << std::endl;
    std::cout << std::endl;

    std::cout << "ЗАДАНИЕ 17: ЗАТЕНЕНИЕ ГУРО" << std::endl;
    task17_gouraud(vertices, polygons, 1200);
    std::cout << std::endl;

    std::cout << "ЗАДАНИЕ 18: ТЕКСТУРИРОВАНИЕ С ОСВЕЩЕНИЕМ" << std::endl;

    std::vector<TexturedVertex> textured_vertices;
    std::vector<std::vector<int>> textured_polygons;
    std::vector<std::vector<int>> texcoord_indices;

    if (!load_obj_with_texture(obj_filename, textured_vertices, textured_polygons)) {
        std::cerr << "Ошибка загрузки модели с текстурными координатами!" << std::endl;
        return 1;
    }

    unsigned int tex_width, tex_height;
    std::vector<unsigned char> texture = load_texture("hi.png", tex_width, tex_height);

    if (!texture.empty()) {
        task18_textured_with_lighting(textured_vertices, textured_polygons,
            texture, tex_width, tex_height, 1200);
    }
    else {
        std::cerr << "Ошибка загрузки текстуры!" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "ВСЕ ЗАДАНИЯ ВЫПОЛНЕНЫ" << std::endl;

    return 0;
} 