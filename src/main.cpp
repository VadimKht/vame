#include <raylib.h>
#include <raymath.h>
#include <cstdio>
#include <cmath>
#include "rlgl.h"

// light configuration
#define MAX_LIGHTS 4

typedef enum {
	LIGHT_DIRECTIONAL = 0,
	LIGHT_POINT
} LightType;

typedef struct Light {
	bool enabled;
	LightType type;
	Vector3 position;
	Vector3 target;
	Color color;
	float intensity;
	// shader locations
	int enabledLoc;
	int typeLoc;
	int posLoc;
	int targetLoc;
	int colorLoc;
	int intensityLoc;
} Light;

void UpdateLightValues(Shader shader, const Light& light);
Light CreateLight(LightType type, Vector3 position, Vector3 target, Color color, float intensity, int lightIndex, Shader shader);

// player configuration
#define PLAYER_WIDTH 0.8f
#define PLAYER_HEIGHT 1.8f
#define MOUSE_SENSITIVITY 0.003f
#define PLAYER_SPEED 8.0f
#define PLAYER_SPRINT_SPEED 16.0f
#define PLAYER_JUMP_FORCE 7.0f
#define GRAVITY -15.0f
#define TIME_SPIN_SPEED 0.1f

// scene object
typedef struct Object {
	Vector3 position;
	Vector3 size;
	Color color;
	bool IsMovingPlatform;
	bool exists;
} Object;

void DrawLine3D(Vector3 startPos, Vector3 endPos, Color color, float thickness) {
    Vector3 direction = Vector3Subtract(endPos, startPos);
    float distance = Vector3Length(direction);
    direction = Vector3Normalize(direction);
    if (distance <= 0.0f)
        return;
    Vector3 up = {0.0f, 1.0f, 0.0f};
    Vector3 rotationAxis = Vector3CrossProduct(direction, up);
    if (Vector3LengthSqr(rotationAxis) < 0.0001f)
        rotationAxis = Vector3CrossProduct(direction, (Vector3){1.0f, 0.0f, 0.0f});
    rotationAxis = Vector3Normalize(rotationAxis);
    float angle = acosf(Vector3DotProduct(up, direction)) * RAD2DEG;
    Vector3 center = Vector3Scale(Vector3Add(startPos, endPos), 0.5f);
    Vector3 size = {thickness, distance, thickness};
    rlPushMatrix();
    rlTranslatef(center.x, center.y, center.z);
    rlRotatef(angle, rotationAxis.x, rotationAxis.y, rotationAxis.z);
    DrawCube((Vector3){0}, thickness, distance, thickness, color);
    rlPopMatrix();
}

void DrawCubeWiresV(Vector3 position, Vector3 size, Color color, float thickness = 0.1f) {
    float x = position.x;
    float y = position.y;
    float z = position.z;
    float halfWidth = size.x / 2.0f;
    float halfHeight = size.y / 2.0f;
    float halfLength = size.z / 2.0f;
    Vector3 v1 = {x - halfWidth, y - halfHeight, z - halfLength};
    Vector3 v2 = {x + halfWidth, y - halfHeight, z - halfLength};
    Vector3 v3 = {x + halfWidth, y + halfHeight, z - halfLength};
    Vector3 v4 = {x - halfWidth, y + halfHeight, z - halfLength};
    Vector3 v5 = {x - halfWidth, y - halfHeight, z + halfLength};
    Vector3 v6 = {x + halfWidth, y - halfHeight, z + halfLength};
    Vector3 v7 = {x + halfWidth, y + halfHeight, z + halfLength};
    Vector3 v8 = {x - halfWidth, y + halfHeight, z + halfLength};
    DrawLine3D(v1, v2, color, thickness);
    DrawLine3D(v2, v3, color, thickness);
    DrawLine3D(v3, v4, color, thickness);
    DrawLine3D(v4, v1, color, thickness);
    DrawLine3D(v5, v6, color, thickness);
    DrawLine3D(v6, v7, color, thickness);
    DrawLine3D(v7, v8, color, thickness);
    DrawLine3D(v8, v5, color, thickness);
    DrawLine3D(v1, v5, color, thickness);
    DrawLine3D(v2, v6, color, thickness);
    DrawLine3D(v3, v7, color, thickness);
    DrawLine3D(v4, v8, color, thickness);
}
bool movePlatforms = false;
int succJumps = 0;

int main(void) {
	// initialization
	const int screenWidth = 1280;
	const int screenHeight = 720;
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "DashRun");

	// player setup
	Camera3D camera = {0};
	camera.position = (Vector3){0.0f, 4.0f, 0.0f};
	camera.up = (Vector3){0.0f, 1.0f, 0.0f};
	camera.fovy = 70.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	Vector3 playerVelocity = {0};
	bool isGrounded = false;
	int groundedOnObjectId = NULL;
	Vector2 cameraRotation = {0};

	// scene setup
	Object scene_Objects[32] = {
		// Walls Spawn
		{{0, 0, 0}, {20, 1, 20}, BLACK, false, true},
		{{-10.6, 5.6, 0}, {1, 10, 20}, BLACK, false, true},
		{{10.6, 5.6, 0}, {1, 10, 20}, BLACK, false, true},
		{{0, 5.6, 10}, {20, 10, 1}, BLACK, false, true},

		// First platforms (Always same, yes)
		// x -5 for left shifted 5 for right shifted
		// z is distance, will be increasing with speed of them moving on their own
		{{-5, 0, -20}, {5, 1, 5}, BLACK, true, true},
		{{5, 0, -40}, {5, 1, 5}, BLACK, true, true},
		{{5, 0, -60}, {5, 1, 5}, BLACK, true, true},
		{{-5, 0, -80}, {5, 1, 5}, BLACK, true, true},
		{{5, 0, -100}, {5, 1, 5}, BLACK, true, true},
		{{-5, 0, -120}, {5, 1, 5}, BLACK, true, true},
		{{-5, 0, -140}, {5, 1, 5}, BLACK, true, true},
	};
	int objectCount = sizeof(scene_Objects) / sizeof(scene_Objects[0]);
	int floorIndex = objectCount - 1;
	float PlatformMovingSpeed = 10;

	// shaders (lighthing)
	Shader shader = LoadShader("../Assets/Lighting.vs", "../Assets/Lighting.fs");
	shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

	// light point
	Light light = CreateLight(LIGHT_POINT, (Vector3){0, 20, 0}, (Vector3){0, 0, 0}, WHITE, 0.8f, 0, shader);

	DisableCursor();

	float time = 0.0f;
	bool closeWindow = false;

	// main loop
	while (!WindowShouldClose() && !closeWindow) {
		float dt = GetFrameTime();
		time += dt;

		// input
		Vector2 mouseDelta = GetMouseDelta();
		cameraRotation.x -= mouseDelta.x * MOUSE_SENSITIVITY; // yaw
		cameraRotation.y -= mouseDelta.y * MOUSE_SENSITIVITY; // pitch

		const float pitchLimit = 1.55f; // Approx 89 degrees
		if (cameraRotation.y > pitchLimit)
			cameraRotation.y = pitchLimit;
		if (cameraRotation.y < -pitchLimit)
			cameraRotation.y = -pitchLimit;

		// update
		// camera rotation calculations
		Quaternion qYaw = QuaternionFromAxisAngle((Vector3){0, 1, 0}, cameraRotation.x);
		Quaternion qPitch = QuaternionFromAxisAngle((Vector3){1, 0, 0}, cameraRotation.y);
		Quaternion qRot = QuaternionMultiply(qYaw, qPitch);

		// movement vectors calculation
		Vector3 forward = Vector3RotateByQuaternion((Vector3){0, 0, -1}, qRot);
		Vector3 right = Vector3RotateByQuaternion((Vector3){1, 0, 0}, qRot);

		// get movement input
		Vector3 moveDir = {0};
		if (IsKeyDown(KEY_W))
			moveDir = Vector3Add(moveDir, forward);
		if (IsKeyDown(KEY_S))
			moveDir = Vector3Subtract(moveDir, forward);
		if (IsKeyDown(KEY_A))
			moveDir = Vector3Subtract(moveDir, right);
		if (IsKeyDown(KEY_D))
			moveDir = Vector3Add(moveDir, right);

		if (Vector3LengthSqr(moveDir) > 0.0f) {
			moveDir.y = 0; // prevent flying
			moveDir = Vector3Normalize(moveDir);
		}

		float currentSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? PLAYER_SPRINT_SPEED : PLAYER_SPEED;
		Vector3 desiredMove = Vector3Scale(moveDir, currentSpeed);

		// gravity and jumping
		playerVelocity.y += GRAVITY * dt;
		if (IsKeyDown(KEY_SPACE) && isGrounded) {
			playerVelocity.y = PLAYER_JUMP_FORCE;
			isGrounded = false;
		}

		// simple collision detection
		BoundingBox playerBox = {
			{camera.position.x - PLAYER_WIDTH / 2, camera.position.y - PLAYER_HEIGHT, camera.position.z - PLAYER_WIDTH / 2},
			{camera.position.x + PLAYER_WIDTH / 2, camera.position.y, camera.position.z + PLAYER_WIDTH / 2}};

		// resolve X
		camera.position.x += desiredMove.x * dt;
		playerBox.min.x += desiredMove.x * dt;
		playerBox.max.x += desiredMove.x * dt;
		for (int i = 0; i < objectCount; i++) {
			BoundingBox objBox = {Vector3Subtract(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f)), Vector3Add(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f))};
			if (CheckCollisionBoxes(playerBox, objBox)) {
				camera.position.x -= desiredMove.x * dt;
				playerBox.min.x -= desiredMove.x * dt;
				playerBox.max.x -= desiredMove.x * dt;
				break;
			}
		}
		// resolve Z
		camera.position.z += desiredMove.z * dt;
		playerBox.min.z += desiredMove.z * dt;
		playerBox.max.z += desiredMove.z * dt;
		for (int i = 0; i < objectCount; i++) {
			BoundingBox objBox = {Vector3Subtract(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f)), Vector3Add(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f))};
			if (CheckCollisionBoxes(playerBox, objBox)) {
				camera.position.z -= desiredMove.z * dt;
				playerBox.min.z -= desiredMove.z * dt;
				playerBox.max.z -= desiredMove.z * dt;
				break;
			}
		}
		// resolve Y

		bool wasGrounded = isGrounded;
		isGrounded = false;
		camera.position.y += playerVelocity.y * dt;
		playerBox.min.y += playerVelocity.y * dt;
		playerBox.max.y += playerVelocity.y * dt;
		for (int i = 0; i < objectCount; i++) {
			BoundingBox objBox = {Vector3Subtract(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f)), Vector3Add(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f))};
			if (CheckCollisionBoxes(playerBox, objBox)) {
				//camera.position.y -= playerVelocity.y * dt;

				// if i dont add 0.001 then the player will be inside the floor which makes player stuck in floor >:()
				camera.position.y = scene_Objects[i].position.y + scene_Objects[i].size.y/2+PLAYER_HEIGHT+0.001;
				if (playerVelocity.y < 0)
				{
					isGrounded = true;
					groundedOnObjectId = i;
				}
				playerVelocity.y = 0;

				// Initial delete spawn when you jump on movable floor
				// how do you think, should i just have a function for it instead?
				if(!movePlatforms && scene_Objects[i].IsMovingPlatform)
				{
					// delete spawn
					scene_Objects[0] = {0};
					scene_Objects[1] = {0};
					scene_Objects[2] = {0};
					scene_Objects[3] = {0};
					movePlatforms = true;
				};
				break;
			}
		}
		
		// if player landed on a moving plawltform
		if(movePlatforms && (wasGrounded  != isGrounded))
		{
			succJumps += 1;

			// todo: stop plaltforms generate a room put a hitbox inside room which deletes platforms before and starts boss.
			// the hitboxes will pop up as red translucent as warning
			if(succJumps == 50) printf("Successful completion of challenge, boss time.\n");
			PlatformMovingSpeed *= 1.02f;
		}

		// Update positions
		for (int i = 0; i < objectCount; i++)
		{
			// if standing on moving platform and the platforms started moving, move the object back
			if(scene_Objects[i].IsMovingPlatform && movePlatforms) scene_Objects[i].position.z += PlatformMovingSpeed * dt;
			// if object is too far, "respawn" it
			if(scene_Objects[i].position.z >= 20) scene_Objects[i].position.z = -120;
		}
		// Very basic test way to make illusion of player moving with platforms
		if(isGrounded && scene_Objects[groundedOnObjectId].IsMovingPlatform) camera.position.z += PlatformMovingSpeed * dt;

		// camera target update
		camera.target = Vector3Add(camera.position, forward);

		// update light position & values
		light.position.x = sinf(time * TIME_SPIN_SPEED) * 40.0f;
		light.position.z = cosf(time * TIME_SPIN_SPEED) * 40.0f;
		UpdateLightValues(shader, light);
		SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position, SHADER_UNIFORM_VEC3);
		if(camera.position.y <= -20) closeWindow = true;

		// drawing
		BeginDrawing();
		ClearBackground(WHITE);

		BeginMode3D(camera);
		{
			// shadered stuff
			BeginShaderMode(shader);
			{
				for (int i = 0; i < objectCount; i++)
					DrawCubeWiresV(scene_Objects[i].position, scene_Objects[i].size, scene_Objects[i].color, 0.4f);
			}
			EndShaderMode();

			for (int i = 0; i < objectCount; i++)
					DrawCubeV(scene_Objects[i].position, scene_Objects[i].size, WHITE);

			// custom-shaderless stuff
			// DrawGrid(200, 2.0f);
			DrawSphere(light.position, 1.0f, light.color); // light source visualization
		}
		EndMode3D();

		// draw UI
		DrawFPS(10, 10);
		// crosshair
		DrawLine(screenWidth / 2, screenHeight / 2 - 10, screenWidth / 2, screenHeight / 2 + 10, RAYWHITE);
		DrawLine(screenWidth / 2 - 10, screenHeight / 2, screenWidth / 2 + 10, screenHeight / 2, RAYWHITE);

		EndDrawing();
	}

	// termination
	UnloadShader(shader);
	CloseWindow();
	return 0;
}

// light-related functions
Light CreateLight(LightType type, Vector3 position, Vector3 target, Color color, float intensity, int lightIndex, Shader shader) {
	Light light = {0};
	if (lightIndex >= MAX_LIGHTS)
		return light;

	light.enabled = true;
	light.type = type;
	light.position = position;
	light.target = target;
	light.color = color;
	light.intensity = intensity;

	char enabledName[32], typeName[32], posName[32], targetName[32], colorName[32], intensityName[32];
	snprintf(enabledName, 32, "lights[%d].enabled", lightIndex);
	snprintf(typeName, 32, "lights[%d].type", lightIndex);
	snprintf(posName, 32, "lights[%d].position", lightIndex);
	snprintf(targetName, 32, "lights[%d].target", lightIndex);
	snprintf(colorName, 32, "lights[%d].color", lightIndex);
	snprintf(intensityName, 32, "lights[%d].intensity", lightIndex);

	light.enabledLoc = GetShaderLocation(shader, enabledName);
	light.typeLoc = GetShaderLocation(shader, typeName);
	light.posLoc = GetShaderLocation(shader, posName);
	light.targetLoc = GetShaderLocation(shader, targetName);
	light.colorLoc = GetShaderLocation(shader, colorName);
	light.intensityLoc = GetShaderLocation(shader, intensityName);

	UpdateLightValues(shader, light);
	return light;
}
void UpdateLightValues(Shader shader, const Light& light) {
	SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
	SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);
	SetShaderValue(shader, light.posLoc, &light.position, SHADER_UNIFORM_VEC3);
	SetShaderValue(shader, light.targetLoc, &light.target, SHADER_UNIFORM_VEC3);

	float colorNormalized[4] = {
		(float)light.color.r / 255.0f,
		(float)light.color.g / 255.0f,
		(float)light.color.b / 255.0f,
		(float)light.color.a / 255.0f};
	SetShaderValue(shader, light.colorLoc, colorNormalized, SHADER_UNIFORM_VEC4);
	SetShaderValue(shader, light.intensityLoc, &light.intensity, SHADER_UNIFORM_FLOAT);
}
