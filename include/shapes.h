#pragma once

#include <raymath.h>

typedef char ShapeType;
enum ShapeTypeOptions
{
    SHP_PIXEL,
    SHP_CIRCLE,
    SHP_LINE
};

typedef struct
{
    Vector2 offset;
} ShapePixel;

typedef struct
{
    Vector2 offset;
    float radius;
} ShapeCircle;

typedef struct
{
    Vector2 start;
    Vector2 finish;
} ShapeLine;

typedef struct
{
    union
    {
        ShapePixel pixel;
        ShapeCircle circle;
        ShapeLine line;
    };
    ShapeType type;
} Shape;

typedef struct 
{
    Color color;
    Shape shape;
} DrawShapeComponent;

Shape Shapes_NewPixel(Vector2 offset);
Shape Shapes_NewCircle(Vector2 offset, float radius);
Shape Shapes_NewLine(Vector2 start, Vector2 finish);

void Shapes_Draw(Vector2 *pos, Shape *shape, Color color);
