// TODO, not used yet, made as a way to handle scene cleaner in future

#include <raylib.h>
#include <raymath.h>
#include <cstring>

// scene object
typedef struct Object {
    bool exists;
    Vector3 position;
    Vector3 size;
    Color color;
    Vector3 velocity;
    const char* Name;
    char tag[32];
} Object;

class Scene{

    public:
        Object objects[32];
        // todo: make json representation of objects and add them by json
        void AddObjectsFromJSON(){

        }
        void AddObject(Vector3 position, Vector3 size, Color Color)
        {
            const size_t objects_size = sizeof(objects)/sizeof(objects[0]);
            for(int i = 0; i < objects_size; i++)
            {
                if(!objects[i].exists)
                {
                    objects[i].exists = true;
                    objects[i].position = position;
                    objects[i].size = size;
                    break;
                }
                continue;
            }
        }
        void DeleteObject(int id){
            objects[id] = {0};
        };
        void DeleteObject(char tagname[32])
        {
            const size_t objects_size = sizeof(objects)/sizeof(objects[0]);
            for(int i = 0; i < objects_size; i++)
            {
                if(strcmp(objects[i].tag, tagname)) {
                    objects[i] = {0};
                    return;
                }
            }
        };
    Scene()
    {
        
    }
    private:
        
};