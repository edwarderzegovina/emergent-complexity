#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <random>
#include <raylib.h>

#define CLUSTER_SIZE 3
#define TARGET_FPS 120
#define HEIGHT 950
#define WIDTH 1200
#define SIZE 2
using namespace std;

bool** grid;
bool** nextGrid;
constexpr int n = HEIGHT/SIZE, m = WIDTH/SIZE;

/*
B36/S23 — HighLife
Like Conway's but cells also born at 6 neighbours. Produces a replicator pattern that copies itself.
B3/S12345 — Maze
Creates long winding corridors and maze-like structures that rarely die out.
B3/S45678 — Coral
Grows slowly outward like coral or crystal formations, very stable.
B36/S125 — 2x2
Tends to form 2x2 blocks and larger symmetric structures.
B2/S — Seeds
Nothing survives, but cells explode outward chaotically from any seed before dying out completely.
B25/S4 — Amoeba
Produces organic, blob-like shapes that slowly shift and morph.
B1357/S1357 — Replicator
Everything replicates — any pattern will copy itself repeatedly across the grid.
B3678/S34678 — Day & Night
Symmetric — dead and alive are interchangeable. Dense regions and sparse regions behave the same way.
B34/S34 — 34 Life
Produces a mix of chaotic growth and stable blobs, slightly more explosive than Conway's.
Stains — B3678/S235678 (spreads like ink blots)
Anneal — B4678/S35678 (smooths out into blobs, like annealing metal)
Diamoeba — B35678/S5678 (grows diamond-shaped amoeba patterns)
Long Life — B345/S5 (very slow, sparse, long-lived structures)
Gnarl — B1/S1 (one of the most chaotic, fractal-like explosions)
Flakes — B3/S012345678 (nothing ever dies, grows like snowflakes)
Serviettes — B2345/S (everything dies but produces lace-like patterns before going)
Walled Cities — B45678/S2345 (forms dense city-block structures surrounded by walls)
Coagulations — B378/S235678 (clumps together into large irregular masses)
Assimilation — B345/S4567 (absorbs everything into stable blobs)
Bloomerang — B234/S3456 (blooms outward then collapses back)
Flock — B3/S12 (small groups that move and flock together)
Land Rush — B35678/S235678 (rapid territorial expansion)
Electrified Maze — B45/S12345 (mazes with sparking live edges)
Persian Rug — B234/S/S0123456 (tiles into intricate symmetric carpet-like patterns)
Burst — B36/S2 (tiny bursts that die almost immediately)
Slow Blob — B367/S345678 (a massive slow-moving blob)
Fireworks — B2/S45678 (explodes outward in sparks then stabilizes into rings)
Crystal Growth — B2356/S45678 (grows in sharp angular crystal formations)
*/

unordered_set<int> reproductionSet = {3,6,7,8};
unordered_set<int> survivalSet = {3,4,6,7,8};

void randomizeGridClustered() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> rowDist(1, n);
    uniform_int_distribution<> colDist(1, m);
    uniform_int_distribution<> chanceDist(0, 99);

    uniform_int_distribution<> clusterCountDist(5, 25);
    uniform_int_distribution<> radiusDist(3, 10);
    uniform_int_distribution<> densityDist(30, 80);

    int numClusters  = clusterCountDist(gen);
    int clusterRadius = radiusDist(gen);
    int density      = densityDist(gen);

    for (int c = 0; c < numClusters; c++) {
        int seedRow = rowDist(gen);
        int seedCol = colDist(gen);

        for (int i = -clusterRadius; i <= clusterRadius; i++) {
            for (int j = -clusterRadius; j <= clusterRadius; j++) {
                int r   = seedRow + i;
                int col = seedCol + j;
                if (r >= 1 && r <= n && col >= 1 && col <= m) {
                    float dist2center = sqrt(i*i + j*j) / clusterRadius;
                    if (chanceDist(gen) < (1.0f - dist2center) * density)
                        grid[r][col] = true;
                }
            }
        }
    }
}

void randomizeGrid() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, 1);

    for (size_t i = 1; i <= n; i++)
        for (size_t j = 1; j <= m; j++)
            grid[i][j] = dist(gen);
}

void createGrid(void){
    grid = new bool*[n+2];
    nextGrid = new bool*[n+2];
    for(size_t i = 0; i < n+2; i++)
        grid[i] = new bool[m+2]{false};
    for(size_t i = 0; i < n+2; i++)
        nextGrid[i] = new bool[m+2]{false};
}

void deleteAllocatedGrid(void){
    for(size_t i = 0; i < n+2; i++)
        delete[] grid[i];
    for(size_t i = 0; i < n+2; i++)
        delete[] nextGrid[i];
    delete[] grid;
    delete[] nextGrid;
}

int getAliveCells(int i, int j){
    auto wrapRow = [](int row) {
        if (row < 1) return n;
        if (row > n) return 1;
        return row;
    };
    auto wrapCol = [](int col) {
        if (col < 1) return m;
        if (col > m) return 1;
        return col;
    };

    int up = wrapRow(i - 1), down = wrapRow(i + 1);
    int left = wrapCol(j - 1), right = wrapCol(j + 1);

    return (grid[up][left]?1:0) + (grid[up][j]?1:0) + (grid[up][right]?1:0)
         + (grid[i][left]?1:0)                      + (grid[i][right]?1:0)
         + (grid[down][left]?1:0) + (grid[down][j]?1:0) + (grid[down][right]?1:0);
}

void drawGrid_(void){
    /*
        for(size_t i = 0; i < n; i++)
            DrawLine(0, i*SIZE, WIDTH, i*SIZE, WHITE);
        for(size_t i = 0; i < m; i++)
            DrawLine(i*SIZE, 0, i*SIZE, HEIGHT, WHITE);
    */
    for(size_t i = 1; i <= n; i++)
        for(size_t j = 1; j <= m; j++)
            if(grid[i][j])
                DrawRectangle((j-1)*SIZE, (i-1)*SIZE, SIZE, SIZE, WHITE);  // fixed x/y swap
}

void placeLine(void){
	for(size_t i = 100;i<300;i++)
		for(size_t j=100;j<200;j++)
			grid[j][i]=true;
}

void placeCells(void){
    bool brushMode = false;
    int clusterSize = CLUSTER_SIZE;
    while(!IsKeyDown(KEY_Q) && !WindowShouldClose()){
        if(IsKeyPressed(KEY_R)){
            randomizeGrid();
        }
	    if(IsKeyPressed(KEY_L))
		    placeLine();
        if(IsKeyPressed(KEY_T)){
            randomizeGridClustered();
        }
        if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_RIGHT))
            clusterSize = min(clusterSize + 1, min(n, m));
        if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_LEFT))
            clusterSize = max(clusterSize - 1, 1);
        BeginDrawing();
        ClearBackground(BLACK);
        drawGrid_();
        DrawText("Place cells, press Q to start", 10, 10, 20, GRAY);
        DrawText(TextFormat("Cluster size: %d", clusterSize), 10, 35, 20, SKYBLUE);
        if(brushMode)
            DrawText("Brush mode: True", 10, 60, 20, GRAY);
        else
            DrawText("Brush mode: False", 10, 60, 20, GRAY);
        EndDrawing();
        if(IsKeyPressed(KEY_A))
            brushMode = (brushMode)?false:true;
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !brushMode){
            int row = GetMouseY() / SIZE + 1;
            int col = GetMouseX() / SIZE + 1;
            if(row >= 1 && row <= n && col >= 1 && col <= m)
                grid[row][col] = true;
        }
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && brushMode){
            int row = GetMouseY() / SIZE + 1;
            int col = GetMouseX() / SIZE + 1;

            int startRow = row - (clusterSize / 2);
            int startCol = col - (clusterSize / 2);
            int endRow = startRow + clusterSize - 1;
            int endCol = startCol + clusterSize - 1;

            if(startRow < 1){
                endRow += 1 - startRow;
                startRow = 1;
            }
            if(startCol < 1){
                endCol += 1 - startCol;
                startCol = 1;
            }
            if(endRow > n){
                startRow -= endRow - n;
                endRow = n;
            }
            if(endCol > m){
                startCol -= endCol - m;
                endCol = m;
            }

            startRow = max(1, startRow);
            startCol = max(1, startCol);
            endRow = min(n, endRow);
            endCol = min(m, endCol);

            for(int i = startRow; i <= endRow; i++)
                for(int j = startCol; j <= endCol; j++)
                    grid[i][j] = true;
        }
    }
}

void makeIteration(void){
    for(size_t i = 1; i <= n; i++){
        for(size_t j = 1; j <= m; j++){
            int alive = getAliveCells(i, j);
            if(grid[i][j])
                //next[i][j] = (alive <= overPopulationc && alive >= underPopulationC);  // survival
                nextGrid[i][j] = survivalSet.count(alive);
            else
                //next[i][j] = (alive == reproducionC);                // reproduction
                nextGrid[i][j] = reproductionSet.count(alive);
        }
    }

    swap(grid, nextGrid);
}

int main(){
    createGrid();
    InitWindow(WIDTH, HEIGHT, "Game Of Life");
    placeCells();
    SetTargetFPS(TARGET_FPS);
    bool paused = false;
    while(!WindowShouldClose()){
        if(IsKeyPressed(KEY_SPACE))
            paused = !paused;

        if(!paused)
            makeIteration();

        BeginDrawing();
        ClearBackground(BLACK);
        drawGrid_();
        DrawText("Q: quit", 10, 10, 20, GRAY);
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 35, 20, GREEN);
        DrawText(paused ? "Paused" : "Running", 10, 60, 20, paused ? ORANGE : LIME);
        EndDrawing();
        if(IsKeyPressed(KEY_Q))
        	break;
    }

    CloseWindow();
    deleteAllocatedGrid();
    return 0;
}
