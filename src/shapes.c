#include <raylib.h>
#include <raymath.h>

#include <shapes.h>

Shape Shapes_NewPixel(Vector2 offset)
{
    return (Shape) {
        .type = SHP_PIXEL,
        .pixel = (ShapePixel) { offset }
    };
}

Shape Shapes_NewCircle(Vector2 offset, float radius)
{
    return (Shape) {
        .type = SHP_CIRCLE,
        .circle = (ShapeCircle) { offset, radius }
    };
}
Shape Shapes_NewCircle_0(float radius) { return Shapes_NewCircle( Vector2Zero(), radius ); }

Shape Shapes_NewLine(Vector2 start, Vector2 finish)
{
    return (Shape) {
        .type = SHP_LINE,
        .line = (ShapeLine) { start, finish }
    };
}
Shape Shapes_NewLine_0(Vector2 finish) { return Shapes_NewLine( Vector2Zero(), finish ); }

void Shapes_Draw(Vector2 *pos, Shape *shape, Color color) 
{
    switch (shape->type) 
    {
        case SHP_PIXEL:
            DrawPixelV(*pos, color);
            break;
        case SHP_CIRCLE:
            DrawCircleV(*pos, shape->circle.radius, color);
            break;
        case SHP_LINE:
            DrawLineV(
                Vector2Add(*pos, shape->line.start), 
                Vector2Add(*pos, shape->line.finish), 
                color
            );
            break;
    }
}
