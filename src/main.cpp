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

// scene object
typedef struct Object {
	Vector3 position;
	Vector3 size;
	Color color;
} Object;

int main(void) {
	// initialization
	const int screenWidth = 1280;
	const int screenHeight = 720;
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "DashRun");

	// player setup
	Camera3D camera = {0};
	camera.position = (Vector3){-10.0f, 4.0f, 0.0f};
	camera.up = (Vector3){0.0f, 1.0f, 0.0f};
	camera.fovy = 70.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	Vector3 playerVelocity = {0};
	bool isGrounded = false;
	Vector2 cameraRotation = {0};

	// scene setup
	Object scene_Objects[] = {
		{{10, 2.5f, 0}, {0.5f, 0.1f, 0.5f}, ORANGE},
		{{0, 0, 10}, {2, 3.0f, 0.5f}, SKYBLUE},
		{{10, 0, 10}, {2, 11.0f, 2}, SKYBLUE},
		{{20, 0, 0}, {0.5f, 2.0f, 2}, PURPLE},
		{{0, 0, 20}, {2, 1.0f, 0.5f}, PURPLE},
		{{30, 0, 10}, {0.5f, 11.0f, 0.5f}, LIME},
		{{0, -0.5f, 0}, {200, 1, 200}, GRAY},
	};
	int objectCount = sizeof(scene_Objects) / sizeof(scene_Objects[0]);
	int floorIndex = objectCount - 1;

	// shaders (lighthing)
	Shader shader = LoadShader("../Assets/Lighting.vs", "../Assets/Lighting.fs");
	shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

	// light point
	Light light = CreateLight(LIGHT_POINT, (Vector3){0, 20, 0}, (Vector3){0, 0, 0}, WHITE, 1.2f, 0, shader);

	DisableCursor();

	float time = 0.0f;

	// main loop
	while (!WindowShouldClose()) {
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
		isGrounded = false;
		camera.position.y += playerVelocity.y * dt;
		playerBox.min.y += playerVelocity.y * dt;
		playerBox.max.y += playerVelocity.y * dt;
		for (int i = 0; i < objectCount; i++) {
			BoundingBox objBox = {Vector3Subtract(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f)), Vector3Add(scene_Objects[i].position, Vector3Scale(scene_Objects[i].size, 0.5f))};
			if (CheckCollisionBoxes(playerBox, objBox)) {
				camera.position.y -= playerVelocity.y * dt;
				if (playerVelocity.y < 0)
					isGrounded = true;
				playerVelocity.y = 0;
				break;
			}
		}

		// camera target update
		camera.target = Vector3Add(camera.position, forward);

		// update light position & values
		light.position.x = sinf(time) * 40.0f;
		light.position.z = cosf(time) * 40.0f;
		UpdateLightValues(shader, light);
		SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position, SHADER_UNIFORM_VEC3);

		// drawing
		BeginDrawing();
		ClearBackground(WHITE);

		BeginMode3D(camera);
		{
			// shadered stuff
			BeginShaderMode(shader);
			{
				for (int i = 0; i < objectCount; i++)
					DrawCubeV(scene_Objects[i].position, scene_Objects[i].size, scene_Objects[i].color);
			}
			EndShaderMode();

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
