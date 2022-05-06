#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "pipeline.h"
#include "camera.h"

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768

GLuint VBO; //назначила GLuint в качестве глобальной переменной для хранения указателя на буфер вершин
GLuint IBO; //добавляем еще один указатель на буферный объект для буфера индексов.
GLuint gWVPLocation; //указатель для доступа к всемирной матрице

Camera* pGameCamera = NULL;  //указатель на камеру

/*создадаём шейдерную программу*/
static const char* pVS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
layout (location = 0) in vec3 Position;                                             \n\
                                                                                    \n\
uniform mat4 gWVP;                                                                  \n\
                                                                                    \n\
out vec4 Color;                                                                     \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    gl_Position = gWVP * vec4(Position, 1.0);                                       \n\
    Color = vec4(clamp(Position, 0.0, 1.0), 1.0);                                   \n\
}";

static const char* pFS = "                                                          \n\
#version 330                                                                        \n\
                                                                                    \n\
in vec4 Color;                                                                      \n\
                                                                                    \n\
out vec4 FragColor;                                                                 \n\
                                                                                    \n\
void main()                                                                         \n\
{                                                                                   \n\
    FragColor = Color;                                                              \n\
}";

static void RenderSceneCB() //функция отображения на экран
{
    pGameCamera->OnRender(); //в функции рендера вызваем камеру. 
    //Это дает ей шанс для действий, если мышь не двигалась, но находится около границы экрана.

    glClear(GL_COLOR_BUFFER_BIT); //Очистка буфера кадра

     /*используем статическую переменную типа float, которую мы будем по-немного увеличивать каждый вызов функции рендера*/
    static float Scale = 0.0f;

    Scale += 0.1f;

    //Эти изменения в функции рендера. Мы создаем объект конвейера, настраиваем его и отправляем результат в шейдер.
    Pipeline p;
    p.Rotate(0.0f, Scale, 0.0f);
    p.WorldPos(0.0f, 0.0f, 3.0f);
    p.SetCamera(pGameCamera->GetPos(), pGameCamera->GetTarget(), pGameCamera->GetUp()); //атрибуты камеры посылаются из класса камеры.
    p.SetPerspectiveProj(60.0f, WINDOW_WIDTH, WINDOW_HEIGHT, 1.0f, 100.0f); //В функции рендера мы назначаем параметры проекции.

    glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, (const GLfloat*)p.GetTrans()); //загружаем матрицу в шейдер

    glEnableVertexAttribArray(0); //Координаты вершин, используемые в буфере, рассматриваются как атрибут вершины с индексом 0 в фиксированной функции конвейера
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //обратно привязываем наш буфер, приготавливая его для отрисовки
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //Этот вызов говорит конвейеру как воспринимать данные внутри буфера
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0); //используем glDrawElements вместо glDrawArrays
    //тип примитивов, какое количество индексов в индексном буфере будет использоваться, тип переменных в буфере индексов
    //Последний параметр передает GPU смещение в байтах от начала буфера индексов до до позиции первого индекса для обработки.

    glDisableVertexAttribArray(0); //Отключаем каждый атрибут вершины

    glutSwapBuffers(); //меняем фоновый буфер и буфер кадра местами
}

//Здесь мы регистрируем новую функцию обратного вызова для получения специальных событий клавиатуры. 
//Функция отправляет клавишу и позицию курсора в момент нажатия кнопки. 
//Мы игнорируем координаты курсора и отправляем событие в нашу камеру, которая уже объявлена как глобальная переменная.
static void SpecialKeyboardCB(int Key, int x, int y)
{
    pGameCamera->OnKeyboard(Key);
}

//мы используем полноэкранный режим, поэтому может возникнуть проблема с выходом из приложения
static void KeyboardCB(unsigned char Key, int x, int y)
{
    //При нажатии 'q' мы выходим
    switch (Key) {
    case 'q':
        exit(0);
    }
}

static void PassiveMouseCB(int x, int y)
{
    pGameCamera->OnMouse(x, y); //Обратный вызов для мыши просто передает ее координаты в камеру.
}

static void InitializeGlutCallbacks()
{
    glutDisplayFunc(RenderSceneCB); //Функция обратного вызова, которая отрисовывает 1 кадр
    glutIdleFunc(RenderSceneCB); //указываем функцию рендера в качестве ленивой
    glutSpecialFunc(SpecialKeyboardCB);
    //регистрируем 2 новых функции обратного вызова
    glutPassiveMotionFunc(PassiveMouseCB); //для мыши 
    glutKeyboardFunc(KeyboardCB); //для нажатия специальных клавиш (это клавиши направление и функциональные клавиши)
}

static void CreateVertexBuffer()
{
    //увеличили массив, что бы он мог содержать 4 вершины
    Vector3f Vertices[4]; //создаем массив из 4 экземпляров структуры Vector3f (этот тип объявлен в math_3d.h) 
    Vertices[0] = Vector3f(-1.0f, -1.0f, 0.5773f);
    Vertices[1] = Vector3f(0.0f, -1.0f, -1.15475);
    Vertices[2] = Vector3f(1.0f, -1.0f, 0.5773f);
    Vertices[3] = Vector3f(0.0f, 1.0f, 0.0f);

    glGenBuffers(1, &VBO); //glGen* функция для генерации объектов переменных типов
                           //Принимает 2 параметра: первый определяет количество объектов 
                           //второй ссылка на массив типа GLuints для хранения указателя, по которому будут храниться данные
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Привязываем указатель к названию цели и затем запускаем команду на цель
                                       //Параметр GL_ARRAY_BUFFER означает, что буфер будет хранить массив вершин
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW); //После связывания нашего объекта, мы наполняем его данными
    //принимает название цели, размер данных в байтах, адрес массива вершин, и флаг, который обозначает использование паттернов для этих данных.
}

//Буфер индексов заполняется с помощью массива индексов. Индексы указывают на расположение вершин в вершинном буфере.
static void CreateIndexBuffer()
{
    unsigned int Indices[] = { 0, 3, 1,
                               1, 3, 2,
                               2, 3, 0,
                               0, 2, 1 };
    //создаем, а затем заполняем буфер индексов используя массив индексов. 
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO); //В дополнении к привязыванию вершинного буфера мы делаем то же самое с индексным.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    GLuint ShaderObj = glCreateShader(ShaderType); //начинаем процесс разработки шейдеров через создание программного объекта

    /*проверяем ошибки*/
    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
    /*создаём шейдер*/
    const GLchar* p[1];
    p[0] = pShaderText;
    GLint Lengths[1];
    Lengths[0] = strlen(pShaderText);
    glShaderSource(ShaderObj, 1, p, Lengths);
    glCompileShader(ShaderObj);
    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }

    glAttachShader(ShaderProgram, ShaderObj);  //присоединяем скомпилированный объект шейдера к объекту программы
}

static void CompileShaders()
{
    GLuint ShaderProgram = glCreateProgram();

    if (ShaderProgram == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

    AddShader(ShaderProgram, pVS, GL_VERTEX_SHADER);
    AddShader(ShaderProgram, pFS, GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };

    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glValidateProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }

    glUseProgram(ShaderProgram);

    gWVPLocation = glGetUniformLocation(ShaderProgram, "gWVP");
    assert(gWVPLocation != 0xFFFFFFFF);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv); //Инициализируем GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA); //Настраиваются некоторые опции GLUT. GLUT_DOUBLE включает двойную буферизацию и буфер цвета
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Tutorial 15");
    glutGameModeString("1280x1024@32"); //Эта функция glut'а разрешает вашему приложению запускаться в полноэкранном режиме, называемом как 'игровой режим'. 
    glutEnterGameMode();

    InitializeGlutCallbacks();  //присоединяем функцию RenderSceneCB к GLUT

    pGameCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT); //Камера теперь автоматически установится в нужное положение, 
    //так как в конструкторе она установит положение курсора в центре экрана (используя функцию glutWarpPointer). 
    //Этот вызов вернет код ошибки если glut еще не был инициализирован.

   /*Инициализируем GLEW и проверяем на ошибки*/
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return 1;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); //Вызов, который устанавливает цвет, который будет использован во время очистки буфера кадра

    CreateVertexBuffer(); //создаем вершины 
    CreateIndexBuffer();

    CompileShaders();

    glutMainLoop(); //Этот вызов передаёт контроль GLUT'у, который теперь начнёт свой собственный цикл. будет вызывать только функцию отображения на экран

    return 0;
}