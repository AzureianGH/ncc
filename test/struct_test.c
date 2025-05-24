// Test file for struct support in NCC
// This shows how to declare and use structs

struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
};

int main() {
    // Simple struct instance
    struct Point p;
    p.x = 10;
    p.y = 20;
    
    // Struct with nested structs
    struct Rectangle rect;
    rect.top_left.x = 0;
    rect.top_left.y = 0;
    rect.bottom_right.x = 100;
    rect.bottom_right.y = 200;
    
    // Struct pointer access
    struct Point* pp = &p;
    pp->x += 5;  // p.x = 15
    
    // Size of structs
    int point_size = sizeof(struct Point);      // Should be 4
    int rect_size = sizeof(struct Rectangle);   // Should be 8
    
    // Return sum of coordinates to test
    return p.x + p.y + rect.bottom_right.x + rect.bottom_right.y;  // 15 + 20 + 100 + 200 = 335
}
