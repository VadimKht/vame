#include "raylib.h"
#include <stdio.h>
#include <math.h>
#include <cstring>
#include <raymath.h>
#include "network.h"


char currentText[256];
void AssignText(char* text){
    strncpy(currentText, text, sizeof currentText - 1);
}
bool CheckCollissionXZ(Vector3 pos1, Vector3 pos2, float size, float speed = 1.0){

    Vector3 coldir = Vector3Scale(Vector3Normalize(Vector3Subtract(pos1, pos2)), speed);
    if(pos1.x > pos2.x-(size/2+coldir.x+1.5) &&
    pos1.x < pos2.x+(size/2+coldir.x+1.5) &&
    pos1.z > pos2.z-(size/2+coldir.y+1.5) &&
    pos1.z < pos2.z+(size/2+coldir.y+1.5) &&

    pos1.y > pos2.y-(size/2) &&
    pos1.y < pos2.y+(size/2+6)) return true;
    return false;
}
bool CheckCollisionY(Vector3 pos1, Vector3 pos2, float size, float speed = 1.0){
    if(pos1.x < pos2.x+(size/2+speed+1.5) &&
    pos1.x > pos2.x-(size/2+speed+1.5) &&
    pos1.z < pos2.z+(size/2+speed+1.5) &&
    pos1.z > pos2.z-(size/2+speed+1.5) &&

    pos1.y > pos2.y+(size/2+speed+5) &&
    pos1.y < pos2.y+(size/2+speed+6)) return true;
    return false;
}
int main(void)
{
    //InitializeNetwork();
    InitWindow(800, 450, "dashrun");
    Camera3D camera = { 0 };
    SetTargetFPS(30);
    camera.position = (Vector3){ -10.0f, 0.0f, 0.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 70.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    float speed = 0.01f;
    Vector3 cubePos = {
        (float)0,
        (float)0,
        (float)0
    };
    DisableCursor();
    Vector2 mp1 = GetMousePosition();
    Vector2 mp2 = GetMousePosition();
    Vector2 CameraRotation = {0.0f, 0.0f};
    typedef struct{
        Vector3 Position;
        float size;
    } Object;
    bool inFlight = false;
    float JumpEnergy = 0;
    Object scene_Objects[9] = { {{20,0,20}, 10}, {{20,10,0}, 10}, {{0,20,20}, 10},
                                {{20,30,20}, 10}, {{20,40,0}, 10}, {{0,40,20}, 10},
                                {{40,40,20}, 10}, {{80,40,0}, 10}, {{120,40,20}, 10} };

    Vector3 movedir = {0,0,0};
    bool standingOnTopOfSomething = false;
    while (!WindowShouldClose())
    {
        // Find mouse position difference between current and previous frame
        mp2 = GetMousePosition();
        Vector2 difference = { mp2.x - mp1.x, mp2.y - mp1.y };
        mp1 = mp2;

        CameraRotation.x += difference.x*0.01;
        CameraRotation.y -= difference.y*0.01;

        // 1.5 is pi/2;
        // clamp camera y
        // TODO: learn how to get rid of mirroring whenever looking above this limit
        if(CameraRotation.y > 1.5) CameraRotation.y = 1.5;
        if(CameraRotation.y < -1.5) CameraRotation.y = -1.5;

        //Quaternion rotation = QuaternionFromEuler(0, -CameraRotation.x, -CameraRotation.y);
        //Quaternion qYaw   = QuaternionFromAxisAngle((Vector3){0, 1, 0}, -CameraRotation.x); // yaw around Y
        //Quaternion qPitch = QuaternionFromAxisAngle((Vector3){1, 0, 0}, -CameraRotation.y); // pitch around X
        //Quaternion qrot = QuaternionMultiply(qYaw, qPitch);

        // QuaternionFromEuler order is different so it switched up places
        Quaternion qyawpitch = QuaternionFromEuler(-CameraRotation.y, -CameraRotation.x, 0.0);
        Vector3 rotated = Vector3RotateByQuaternion({0,0,1}, qyawpitch);

        // for movement
        Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        forward.y = 0;
        Vector3 right = Vector3Normalize(Vector3CrossProduct({0,1,0}, forward));
        //Vector3 cameraDirection = Vector3Normalize(forward);

        speed = IsKeyDown(KEY_LEFT_SHIFT) ? 0.1f : 1.0f;
        movedir = {0,0,0};
        if (IsKeyDown(KEY_W)) movedir = Vector3Add(movedir, forward);
        if (IsKeyDown(KEY_S)) movedir = Vector3Subtract(movedir, forward);
        if (IsKeyDown(KEY_A)) movedir = Vector3Add(movedir, right);
        if (IsKeyDown(KEY_D)) movedir = Vector3Subtract(movedir, right);
        movedir = Vector3Normalize(movedir);

        if (IsKeyDown(KEY_E)) DrawText(currentText, 0, 0, 36, RED);
        standingOnTopOfSomething = false;
        BeginDrawing();
            ClearBackground(RAYWHITE);        
            BeginMode3D(camera);
                for(int i = 0; i < sizeof(scene_Objects)/sizeof(scene_Objects[0]); i++)
                {
                    const float size = scene_Objects[i].size;
                    Vector3 colisionDirection = Vector3Subtract(camera.position, scene_Objects[i].Position);
                    // circle of same size
                    //if(Vector3Length(colisionDirection) < size/2+1) 

                    //Vector3 coldir = Vector3Scale(Vector3Normalize(colisionDirection), speed);

                    BoundingBox camobj = BoundingBox(
                        camera.position, camera.position
                    );

                    BoundingBox bbobj = BoundingBox(
                        Vector3Subtract(scene_Objects[i].Position, Vector3(scene_Objects[i].size/2)),
                        Vector3Add(scene_Objects[i].Position, Vector3(scene_Objects[i].size/2))
                    );
                    if(CheckCollisionBoxes(camobj, bbobj)
                    ){
                        Vector3 wallNormal = Vector3Normalize({colisionDirection.x, 0, colisionDirection.z});
                        if (Vector3Length(wallNormal) > 0.01f) {
                            movedir = Vector3Subtract(movedir, Vector3Scale(wallNormal, -1));
                        }
                    }

                    
                    if(//CheckCollisionY(camera.position, scene_Objects[i].Position, scene_Objects[i].size)
                    //||
                    camera.position.y < 0.1f)
                    {
                       standingOnTopOfSomething = true;
                       JumpEnergy = 0;
                    }
                    
                    DrawCube(scene_Objects[i].Position, size, size, size, RED);
                    
                }
                if(standingOnTopOfSomething) inFlight = false;
                else{ inFlight = true; }
                    
                camera.position = Vector3Add(camera.position, Vector3Scale(movedir, speed));

                if (IsKeyDown(KEY_SPACE) && !inFlight){
                    JumpEnergy = 2;
                    inFlight = true;
                }

                if(inFlight){
                    JumpEnergy -= 0.1f;
                    camera.position.y += JumpEnergy;
                } 

                camera.target = Vector3Add(rotated, camera.position);
            EndMode3D();
        EndDrawing();
    }
    CloseWindow();

    return 0;
}
