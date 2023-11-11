#include <SDL.h>
#include <ctime>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <string>

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 600;

class Color {
public:
    uint8_t r, g, b, a;
};

Color clearColor = {0, 0, 0, 255};
Color currentColor = {0, 0, 255, 255};

SDL_Renderer* renderer;

struct Face {
    std::array<int, 3> vertexIndices;
};

void point(int x, int y) {
    SDL_RenderDrawPoint(renderer, x, y);
}

void line(const glm::vec3& start, const glm::vec3& end) {
    SDL_RenderDrawLine(renderer, static_cast<int>(start.x), static_cast<int>(start.y),
                       static_cast<int>(end.x), static_cast<int>(end.y));
}

void triangle(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C) {
    line(A, B);
    line(B, C);
    line(C, A);
}

void render(const std::vector<glm::vec3>& vertex) {
    SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);


    for (size_t i = 0; i < vertex.size(); i += 3) {
        const glm::vec3& A = vertex[i];
        const glm::vec3& B = vertex[i + 1];
        const glm::vec3& C = vertex[i + 2];

        int offsetX = WINDOW_WIDTH / 2;
        int offsetY = WINDOW_HEIGHT / 2;

        triangle(A + glm::vec3(offsetX, offsetY, 0), B + glm::vec3(offsetX, offsetY, 0), C + glm::vec3(offsetX, offsetY, 0));
    }
}

std::vector<glm::vec3> setupVertex(const std::vector<glm::vec3>& vertices, const std::vector<Face>& faces)
{
    std::vector<glm::vec3> vertex;
    float scale = 1.0f;

    for (const auto& face : faces)
    {
        glm::vec3 vertexPosition1 = vertices[face.vertexIndices[0]];
        glm::vec3 vertexPosition2 = vertices[face.vertexIndices[1]];
        glm::vec3 vertexPosition3 = vertices[face.vertexIndices[2]];

        glm::vec3 vertexScaled1 = vertexPosition1 * scale;
        glm::vec3 vertexScaled2 = vertexPosition2 * scale;
        glm::vec3 vertexScaled3 = vertexPosition3 * scale;

        vertex.push_back(vertexScaled1);
        vertex.push_back(vertexScaled2);
        vertex.push_back(vertexScaled3);
    }

    return vertex;
}

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<Face>& out_faces) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open OBJ file: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_vertices;
    std::vector<std::array<int, 3>> temp_faces;

    glm::vec3 centroid(0.0f); // Inicializar el centroide en (0, 0, 0)

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);

            centroid += vertex;
        } else if (type == "f") {
            std::array<int, 3> face_indices{};
            for (int i = 0; i < 3; i++) {
                std::string faceIndexStr;
                iss >> faceIndexStr;

                size_t pos = faceIndexStr.find_first_of('/');
                if (pos != std::string::npos) {
                    faceIndexStr = faceIndexStr.substr(0, pos);
                }

                face_indices[i] = std::stoi(faceIndexStr); // No restar 1
            }
            temp_faces.push_back(face_indices);
        }
    }


    centroid /= static_cast<float>(temp_vertices.size());
    float rotationAngleY = 0.0f;
    glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

    float rotationAngleX = 0.0f;
    glm::mat4 rotationMatrixX = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleX), glm::vec3(1.0f, 0.0f, 0.0f));

    float rotationAngleZ = 0.0f;
    glm::mat4 rotationMatrixZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleZ), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 combinedRotationMatrix = rotationMatrixY * rotationMatrixX * rotationMatrixZ;


    out_vertices.reserve(temp_vertices.size());
    for (const auto& vertex : temp_vertices) {

        glm::vec3 centeredVertex = (vertex - centroid) * 30.0f;

        glm::vec4 rotatedVertex = combinedRotationMatrix * glm::vec4(centeredVertex, 1.0f);

        out_vertices.push_back(glm::vec3(rotatedVertex));
    }

    out_faces.reserve(temp_faces.size() * 2);
    for (const auto& face : temp_faces) {
        if (face.size() == 3) {
            Face f{};
            f.vertexIndices = { face[0] - 1, face[1] - 1, face[2] - 1 };
            out_faces.push_back(f);
        } else if (face.size() == 4) {

            Face f1{}, f2{};
            f1.vertexIndices = { face[0] - 1, face[1] - 1, face[2] - 1 };
            f2.vertexIndices = { face[0] - 1, face[2] - 1, face[3] - 1 };
            out_faces.push_back(f1);
            out_faces.push_back(f2);
        }
    }




    return true;
}

int main(int argc, char* argv[]) {

    SDL_Init(SDL_INIT_EVERYTHING);

    srand(time(nullptr));

    SDL_Window* window = SDL_CreateWindow("Pixel Drawer", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    int renderWidth, renderHeight;
    SDL_GetRendererOutputSize(renderer, &renderWidth, &renderHeight);

    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;

    bool success = loadOBJ("../lab3.obj", vertices, faces);
    if (!success) {
        return 1;
    }

    std::vector<glm::vec3> vertex = setupVertex(vertices, faces);

    bool running = true;
    SDL_Event event;

    float rotationSpeed = 1.0f;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        float rotationAngleY = rotationSpeed;

        glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngleY), glm::vec3(1.0f, 0.0f, 0.0f));

        for (auto& vertex : vertex) {
            glm::vec4 rotatedVertex = rotationMatrixY * glm::vec4(vertex, 1.0f);
            vertex = glm::vec3(rotatedVertex);
        }

        SDL_SetRenderDrawColor(renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
        render(vertex);
        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / 60);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}


