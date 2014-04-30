// fragment shading of sphere model

#include "Angel.h"
#include <iostream>
#include <cstdlib>
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <math.h>

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

// Constants
vec3 PaddlePosInitial(0.0,0.0,-3.0);
vec3 WallPosInitial(0.0,0.0,-13.0);
vec3 BallPosInitial(0.0,0.0,-8.0);

vec3 VelInitial(-0.1,-0.1,0.2);
float VelIncrementZ = 0.04;
float VelMaxZ = 1.2;

float LeftWallX = -10.0;
float RightWallX = 10.0;
float FloorY = -7.0;
float CeilingY = 7.0;
float FloatImprecisionFactor = 0.25;

float DeflectionReductionFactor = 8.0;
int MsPerFrame = 50;
int WindowWidth = 768;
int WindowHeight = 576;

vec3 ballVel(VelInitial.x,VelInitial.y,VelInitial.z);
int score = 0;

struct collisionInfo{
	bool isColliding;
	bool isComingFromPaddle;
	float locationX;	//location of collision on paddle
	float locationY;	//location of collision on paddle

	collisionInfo() : isColliding(false), isComingFromPaddle(false), locationX(0.0), locationY(0.0) {}

} collision;

//for angel sphere
const int NumTimesToSubdivide = 5;
const int NumTriangles        = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumVertices         = 3 * NumTriangles;
point4 points[NumVertices];
vec3   normals[NumVertices];
int Index = 0;

// Model and view matrices uniform location
GLuint  mMatrix, vMatrix, pMatrix;
GLuint vaoP, vaoW, vaoB, eboP, eboW, eboB;
GLuint programP, programW, programB;
mat4 modelP, modelW, modelB;

// Create camera view variables
point4 at( 0.0, 0.0, -1.0, 1.0 );
point4 eye( 0.0, 0.0, 0.0, 1.0 );
vec4   up( 0.0, 1.0, 0.0, 0.0 );

GLfloat positionArray[]={
	// Paddle
	-2.0,-2.0,0.0,
	-2.0,2.0,0.0,
	2.0,2.0,0.0,
	2.0,-2.0,0.0,

	// Wall
	-10.0,-7.0,0.0,
	-10.0,7.0,0.0,
	10.0,7.0,0.0,
	10.0,-7.0,0.0
};

GLfloat colorArray[]={
	// Paddle
	1.0,0.0,0.0,0.5,
	1.0,0.0,0.0,0.5,
	1.0,0.0,0.0,0.5,
	1.0,0.0,0.0,0.5,

	// Wall
	0.0,0.0,1.0,1.0,
	0.0,0.0,1.0,1.0,
	0.0,0.0,1.0,1.0,
	0.0,0.0,1.0,1.0
};

GLubyte elemsArray[]={
	0,1,2,3
};

// Define geometric Constants
GLuint NumVerticies = 4;
GLfloat BallRadius = 0.5;
GLfloat PaddleHeight = 4.0;
GLfloat PaddleWidth = 4.0;

size_t posDataOffset, colorDataOffset, normalsDataOffset, spherePosDataOffset;


// Functional Prototypes
void init( );
void printMat4( mat4 );
void resetGame( );
void display( SDL_Window* );
void input( SDL_Window* );
void updateCollision( );
void updateScore( );
void updateSpeed( );
void updateBallPosition( bool );
void reshape( int, int );

// -----------------------------------------------
// -------------- F U N C T I O N S --------------
// -----------------------------------------------

//for angel sphere
void triangle( const point4& a, const point4& b, const point4& c ){
    vec3  normal = normalize( cross(b - a, c - b) );

    normals[Index] = normal;  points[Index] = a;  Index++;
    normals[Index] = normal;  points[Index] = b;  Index++;
    normals[Index] = normal;  points[Index] = c;  Index++;
}

point4 unit( const point4& p ){
    float len = p.x*p.x + p.y*p.y + p.z*p.z;
    
    point4 t;
    if ( len > DivideByZeroTolerance ) {
	t = p / sqrt(len);
	t.w = 1.0;
    }

    return t;
}

void divide_triangle( const point4& a, const point4& b, const point4& c, int count ){
    if ( count > 0 ) {
        point4 v1 = unit( a + b );
        point4 v2 = unit( a + c );
        point4 v3 = unit( b + c );
        divide_triangle(  a, v1, v2, count - 1 );
        divide_triangle(  c, v2, v3, count - 1 );
        divide_triangle(  b, v3, v1, count - 1 );
        divide_triangle( v1, v3, v2, count - 1 );
    }
    else {
        triangle( a, b, c );
    }
}

void tetrahedron( int count ){
	point4 v[4] = {
		vec4( 0.0, 0.0, 1.0, 1.0 ),
		vec4( 0.0, 0.942809, -0.333333, 1.0 ),
		vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
		vec4( 0.816497, -0.471405, -0.333333, 1.0 )
	};

    divide_triangle( v[0], v[1], v[2], count );
    divide_triangle( v[3], v[2], v[1], count );
    divide_triangle( v[0], v[3], v[1], count );
    divide_triangle( v[0], v[2], v[3], count );
}

// OpenGL initialization
void init(){
	// Load shaders and use the resulting shader program
	programP = InitShader( "vshaderP.glsl", "fshader_nolights.glsl" );
	programW = InitShader( "vshaderW.glsl", "fshader_nolights.glsl" );
	programB = InitShader( "vshaderB.glsl", "fshader_lights.glsl" );

	// Define data members
	GLuint vbo;
	// Subdivide a tetrahedron into a sphere
	tetrahedron( NumTimesToSubdivide );


	// ------------------------------------------------------------------------
	// -------  V E R T E X   A R R A Y   O B J E C T   P A D D L E  -------
	// ------------------------------------------------------------------------
	// Define offsets and sizes
	posDataOffset = 0;
	colorDataOffset = posDataOffset + sizeof(positionArray);
	spherePosDataOffset = colorDataOffset + sizeof(colorArray);
	normalsDataOffset = spherePosDataOffset + sizeof(points);

	// Use programP
	glUseProgram( programP );

	// Generate and bind new vertex array object
	glGenVertexArrays( 1,&vaoP );
	glBindVertexArray( vaoP );

	// Generate and bind new vertex buffer object and populate the buffer
	glGenBuffers( 1,&vbo );
	glBindBuffer( GL_ARRAY_BUFFER,vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof(positionArray) + sizeof(colorArray) + sizeof(points) + 
		sizeof(normals),NULL,GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER,posDataOffset,sizeof(positionArray),positionArray );
	glBufferSubData( GL_ARRAY_BUFFER,colorDataOffset,sizeof(colorArray),colorArray );
	glBufferSubData( GL_ARRAY_BUFFER,spherePosDataOffset,sizeof(points), points );
	glBufferSubData( GL_ARRAY_BUFFER,normalsDataOffset,sizeof(normals), normals );

	// Bind position attribute of vbo
	GLuint in_position = glGetAttribLocation( programP, "in_position" );
	glVertexAttribPointer( in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset) );
	glEnableVertexAttribArray( in_position );

	// Bind color attribute of vbo
	GLuint in_color = glGetAttribLocation( programP, "in_color" );
	glVertexAttribPointer( in_color,4,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(colorDataOffset) );
	glEnableVertexAttribArray( in_color );
	

	// Generate and bind element buffer object
	glGenBuffers( 1,&eboP );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,eboP );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

	// Release bind to vaoP and programP
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	// -------  V E R T E X   A R R A Y   O B J E C T   W A L L  -------
	// --------------------------------------------------------------------
	// Define new offsets
	posDataOffset += sizeof(GLfloat) * 3 * NumVerticies;
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;
	//sphere position data offset (points) stays constant
	//normals data offset (normals) stays constant

	// Use programW
	glUseProgram( programW );

	// Generate new vertex array object
	glGenVertexArrays( 1,&vaoW );
	glBindVertexArray( vaoW );

	// Bind vertex buffer object
	// --Use same vbo as vaoP (no new buffer has been bound)

	// Bind attributes to vertex array
	in_position = glGetAttribLocation( programW, "in_position" );
	glVertexAttribPointer(in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset));
	glEnableVertexAttribArray( in_position );

	in_color = glGetAttribLocation( programW, "in_color" );
	glVertexAttribPointer(in_color,4,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(colorDataOffset));
	glEnableVertexAttribArray( in_color );

	// Generate and bind element buffer object
	glGenBuffers( 1,&eboW );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,eboW );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

	// Release bind to vaoW and programW
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------------

	// --------------------------------------------------------------------
	// -------  V E R T E X   A R R A Y   O B J E C T   B A L L  -------
	// --------------------------------------------------------------------
	// Define new offsets
	posDataOffset += sizeof(GLfloat) * 3 * NumVerticies;
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;
	//sphere position data offset (points) stays constant
	//normals data offset (normals) stays constant

	// Use programB
	glUseProgram( programB );

	// Generate new vertex array object
	glGenVertexArrays( 1,&vaoB );
	glBindVertexArray( vaoB );

	glUseProgram( programB );

	// set up vertex arrays
	in_position = glGetAttribLocation( programB, "in_position" );
	glEnableVertexAttribArray( in_position );
	glVertexAttribPointer( in_position, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(spherePosDataOffset) );

	GLuint in_normals = glGetAttribLocation( programB, "in_normals" ); 
	glEnableVertexAttribArray( in_normals );
	glVertexAttribPointer( in_normals, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(normalsDataOffset));


	// Light stuff for ball------------------------------------------
	// Initialize shader lighting parameters
	point4 light_positionB( 10.0, 10.0, 10.0, 0.0 );
	color4 light_ambientB( 0.2, 0.2, 0.2, 1.0 );
	color4 light_diffuseB( 1.0, 1.0, 1.0, 1.0 );
	color4 light_specularB( 1.0, 1.0, 1.0, 1.0 );

	color4 material_ambientB( 1.0, 0.0, 1.0, 1.0 );
	color4 material_diffuseB( 1.0, 0.8, 0.0, 1.0 );
	color4 material_specularB( 1.0, 0.0, 1.0, 1.0 );
	float  material_shininessB = 5.0;
	
	color4 ambient_productB = light_ambientB * material_ambientB;
	color4 diffuse_productB = light_diffuseB * material_diffuseB;
	color4 specular_productB = light_specularB * material_specularB;

	glUniform4fv( glGetUniformLocation(programB, "AmbientProduct"),
		  1, ambient_productB );
	glUniform4fv( glGetUniformLocation(programB, "DiffuseProduct"),
		  1, diffuse_productB );
	glUniform4fv( glGetUniformLocation(programB, "SpecularProduct"),
		  1, specular_productB );
	
	glUniform4fv( glGetUniformLocation(programB, "LightPosition"),
		  1, light_positionB );
	
	glUniform1f( glGetUniformLocation(programB, "Shininess"),
		 material_shininessB );


	// Release bind to vaoB and programB
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------
		 

	// Retrieve transformation uniform variable locations
	mMatrix = glGetUniformLocation( programP, "modelMatrix" );
	vMatrix = glGetUniformLocation( programP, "viewMatrix" );
	pMatrix = glGetUniformLocation( programP, "projectionMatrix" );

	// Initialize model matrices to their correct positions
	modelP = identity() * Translate(PaddlePosInitial);
	modelW = identity() * Translate(WallPosInitial);
	modelB = identity() * Translate(BallPosInitial);

	// Initialize ball velocity
	ballVel.x = VelInitial.x;
	ballVel.y = VelInitial.y;
	ballVel.z = VelInitial.z;

	glEnable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );

	glClearColor( 1.0, 1.0, 1.0, 1.0 ); // black background
}

//----------------------------------------------------------------------------

void printMat4(mat4 m){
	std::cout<<" "<<m[0][0]<<" "<<m[0][1]<<" "<<m[0][2]<<" "<<m[0][3]<<std::endl;
	std::cout<<" "<<m[1][0]<<" "<<m[1][1]<<" "<<m[1][2]<<" "<<m[1][3]<<std::endl;
	std::cout<<" "<<m[2][0]<<" "<<m[2][1]<<" "<<m[2][2]<<" "<<m[2][3]<<std::endl;
	std::cout<<" "<<m[3][0]<<" "<<m[3][1]<<" "<<m[3][2]<<" "<<m[3][3]<<std::endl;
}

//----------------------------------------------------------------------------

void resetGame(){
	// Reset collision struct
	collision.isColliding = false;
	collision.isComingFromPaddle = false;
	collision.locationX = 0.0;
	collision.locationY = 0.0;

	// Reset score
	score = 0;

	// Reset vao models
	modelP = identity() * Translate(PaddlePosInitial);
	modelW = identity() * Translate(WallPosInitial);
	modelB = identity() * Translate(BallPosInitial);
	ballVel = VelInitial;
	updateBallPosition(true);
}

//----------------------------------------------------------------------------

void display( SDL_Window* screen ){
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Define view
	mat4 view = LookAt( eye, at, up );

	// Draw elements of vaoB
	glUseProgram( programB );
	glBindVertexArray( vaoB );
	glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelB );
	glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);

	// Draw elements of vaoW
	glUseProgram( programW );
	glBindVertexArray( vaoW );
	glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelW );
	glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
	glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	// Draw elements of vaoP
	glUseProgram( programP );
	glBindVertexArray( vaoP );
	glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelP );
	glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
	glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	glDisable(GL_BLEND);
	glDepthMask(1);

	// Release binds and swap buffers
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	glFlush();
	SDL_GL_SwapWindow(screen);
}

//----------------------------------------------------------------------------

void input(SDL_Window* screen){

	SDL_Event event;

	//Handle the keyboard
	while (SDL_PollEvent(&event)){
		switch (event.type){
		case SDL_QUIT:
			exit(EXIT_SUCCESS);
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym){
			case SDLK_ESCAPE:
			case SDLK_q:
				exit(EXIT_SUCCESS);
			case SDLK_w: case SDLK_UP:	// move paddle up
				if (modelP[1][3] < CeilingY - FloatImprecisionFactor) {
					modelP = modelP * Translate(0.0,1.0,0.0);
				}
				break;
			case SDLK_s: case SDLK_DOWN:	// move paddle down;
				if (modelP[1][3] > FloorY + FloatImprecisionFactor) {
					modelP = modelP * Translate(0.0,-1.0,0.0);
				}
				break;
			case SDLK_d: case SDLK_RIGHT:	// move paddle right;
				if (modelP[0][3] < RightWallX - FloatImprecisionFactor) {
					modelP = modelP * Translate(1.0,0.0,0.0);
				}
				break;
			case SDLK_a: case SDLK_LEFT:	// move paddle left;
				if (modelP[0][3] > LeftWallX + FloatImprecisionFactor) {
					modelP = modelP * Translate(-1.0,0.0,0.0);
				}
				break;
			case SDLK_r://new game
				resetGame();
				break;
			}
		case SDL_MOUSEMOTION:
			float MouseMotionFactor = 26.0;

			static int prevMouseX = event.motion.x;
			static int prevMouseY = event.motion.y;

			int mouseX = event.motion.x, mouseY = event.motion.y;

			float proposedMoveX = (mouseX - prevMouseX)/MouseMotionFactor;
			float proposedMoveY = -(mouseY - prevMouseY)/MouseMotionFactor;

			vec3 translationVec3(0.0,0.0,0.0);

			// Control Paddle movement on X axis
			if(modelP[0][3] + proposedMoveX < RightWallX &&
				modelP[0][3] + proposedMoveX > LeftWallX){
				translationVec3.x = translationVec3.x + proposedMoveX;
			}

			// Control Paddle movement on Y axis
			if(modelP[1][3] + proposedMoveY < CeilingY &&
				modelP[1][3] + proposedMoveY > FloorY){
				translationVec3.y = translationVec3.y + proposedMoveY;
			}

			//std::cout<<"Current X="<<modelP[0][3]+PaddleWidth/2+proposedMoveX<<std::endl;
			//std::cout<<"Proposed final X (right)="<<modelP[0][3]+PaddleWidth/2+proposedMoveX<<std::endl;
			//std::cout<<"Proposed final X (center)="<<modelP[0][3]+proposedMoveX<<std::endl;
			//std::cout<<"Proposed move X="<<proposedMoveX<<std::endl;
			//printMat4(modelP);

			// Set up previous mouse coordinates for next MouseMotion event
			prevMouseX = mouseX;
			prevMouseY = mouseY;

			// Move the paddle accordingly
			modelP = modelP * Translate(translationVec3);

			break;
		}
	}
}

//----------------------------------------------------------------------------

void updateCollision(){
	// Get positions
	float paddleLx, paddleRx, paddleBy, paddleTy, paddleFz, paddleNz;
	float ballLx, ballRx, ballBy, ballTy, ballFz, ballNz;
	paddleLx = modelP[0][3] - PaddleWidth/2.0;
	paddleRx = modelP[0][3] + PaddleWidth/2.0;
	paddleBy = modelP[1][3] - PaddleHeight/2.0;
	paddleTy = modelP[1][3] + PaddleHeight/2.0;
	paddleFz = modelP[2][3];
	paddleNz = modelP[2][3];
	ballLx = modelB[0][3] - BallRadius;
	ballRx = modelB[0][3] + BallRadius;
	ballBy = modelB[1][3] - BallRadius;
	ballTy = modelB[1][3] + BallRadius;
	ballFz = modelB[2][3] - BallRadius;
	ballNz = modelB[2][3] + BallRadius;

	vec3 paddlePos(modelP[0][3], modelP[1][3], modelP[2][3]);
	vec3 wallPos(modelW[0][3], modelW[1][3], modelW[2][3]);
	vec3 ballPos(modelB[0][3], modelB[1][3], modelB[2][3]);

	// If collision with paddle
	if (ballLx <= paddleRx && ballRx >= paddleLx &&
		ballBy <= paddleTy && ballTy >= paddleBy &&
		ballFz <= paddleFz && ballNz >= paddleNz &&
		!collision.isComingFromPaddle){

		//std::cout<<"Collision with paddle"<<std::endl;
		// Set collision info
		collision.isColliding=true;
		collision.isComingFromPaddle=true;
		collision.locationX = ballPos.x - paddlePos.x;
		collision.locationY = ballPos.y - paddlePos.y;
	} // If collision with back wall
	else if (ballFz <= wallPos.z && collision.isComingFromPaddle) {
		//std::cout<<"Collision with back wall"<<std::endl;
		// Set collision info

		collision.isColliding=true;
		collision.isComingFromPaddle=false;
		collision.locationX = 0.0;
		collision.locationY = 0.0;
	} // No present collisions
	else {
		collision.isColliding=false;
	}

	/////////////////////////////////////////////////////////
	// Left/right and floor/ceiling collisions are checked //
	//   in separate if blocks because they can happen     //
	//   simultaneously.                                   //
	/////////////////////////////////////////////////////////

	// Check for left/right wall collision
	if ((ballLx <= LeftWallX && ballVel.x < 0) ||
		(ballRx >= RightWallX && ballVel.x > 0)){

		//std::cout<<"Collision with left/right wall"<<std::endl;
		ballVel.x = -ballVel.x;
	}

	// Check for floor/ceiling collision
	if ((ballBy <= LeftWallX && ballVel.y < 0) ||
		(ballTy >= RightWallX && ballVel.y > 0)){

		//std::cout<<"Collision with floor/ceiling"<<std::endl;
		//printMat4(modelB);
		ballVel.y = -ballVel.y;
	}
}

//----------------------------------------------------------------------------

void updateScore(){
	float GoalDepthZ = 1.0;

	float ballPositionZ = modelB[2][3];
	float missWall = modelP[2][3] + GoalDepthZ;

	// Player hit the ball
	if (collision.isColliding && collision.isComingFromPaddle){
		score++;
		std::cout<<"Score: "<<score<<std::endl;
	}
	// Player loses
	if (ballPositionZ >= missWall){
		std::cout<<"Player missed with a score of "<<score<<"!"<<std::endl;
		resetGame();
		//Debug-cts
		//collision.isColliding=true;
		//collision.isComingFromPaddle=true;
		//collision.locationX = 0.0;
		//collision.locationY = 0.0;
		//Debug-cts
	}
}

//----------------------------------------------------------------------------

void updateSpeed(){
	// Player hits the ball
	if (collision.isColliding && collision.isComingFromPaddle){
		// Ball is not travelling at max speed
		if (ballVel.z <= VelMaxZ) {
			ballVel.z = ballVel.z + VelIncrementZ;
		}
	}
}

//----------------------------------------------------------------------------

void updateBallPosition(bool forceReset){
	// Reset to initial ball velocities
	if (forceReset){
		ballVel.x = VelInitial.x;
		ballVel.y = VelInitial.y;
		ballVel.z = VelInitial.z;
	}	// Collision detected - update trajectory
	else if (collision.isColliding){
		// Deflect if collision with paddle
		if (collision.isComingFromPaddle){
			ballVel.x = collision.locationX/DeflectionReductionFactor;
			ballVel.y = collision.locationY/DeflectionReductionFactor;
		}
		ballVel.z = -ballVel.z;
	}

	// Translate ball based on ball's velocity
	modelB = modelB * Translate(ballVel.x, ballVel.y, ballVel.z);
}

//----------------------------------------------------------------------------

void reshape( int width, int height ){
	glViewport( 0, 0, width, height );

	GLfloat zNearPersp = abs(modelP[2][3])-1.0, zFarPersp = modelW[2][3]-1.0;
	GLfloat FovY = 150.0;

	GLfloat aspect = GLfloat(width)/height;

	mat4 projection = Perspective( FovY, aspect, zNearPersp, zFarPersp );

	// Bind new projection to each program
	glUseProgram( programP );
	glUniformMatrix4fv( pMatrix, 1, GL_TRUE, projection );

	glUseProgram( programB );
	glUniformMatrix4fv( pMatrix, 1, GL_TRUE, projection );
	glUseProgram( 0 );
}

//----------------------------------------------------------------------------

int main( int argc, char **argv )
{
	//SDL window and context management
	SDL_Window *window;

	//used in main loop
	int sleepTime = 0;
	int ticksBegin, ticksEnd;

	if(SDL_Init(SDL_INIT_VIDEO)<0){//initilizes the SDL video subsystem
		fprintf(stderr,"Unable to create window: %s\n", SDL_GetError());
		SDL_Quit();
		exit(EXIT_FAILURE);//die on error
	}

	//create window
	window = SDL_CreateWindow(
		"Beamer's Crew - Project 2",	//Window title
		SDL_WINDOWPOS_UNDEFINED,	//initial x position
		SDL_WINDOWPOS_UNDEFINED,	//initial y position
		WindowWidth,				//width, in pixels
		WindowHeight,				//height, in pixels
		SDL_WINDOW_OPENGL			//flags to be had
		);

	//check window creation
	if(window==NULL){
		fprintf(stderr,"Unable to create window: %s\n",SDL_GetError());
	}

	//creates opengl context associated with the window
	SDL_GLContext glcontext=SDL_GL_CreateContext(window);

	//initializes glew
	glewExperimental=GL_TRUE;
	if(glewInit()){
		fprintf(stderr, "Unable to initalize GLEW");
		exit(EXIT_FAILURE);
	}

	init();

	// ---------------------------------------------
	// ------------- M A I N   L O O P -------------
	// ---------------------------------------------
	while (true) {
		//For frame management
		ticksBegin = SDL_GetTicks();

		// Listen for keyboard input
		input(window);

		// Update frame to frame positioning
		updateCollision();
		updateScore();
		updateSpeed();
		updateBallPosition(false);
		reshape(WindowWidth,WindowHeight);
		display(window);

		// Frame rate management
		ticksEnd = SDL_GetTicks();
		sleepTime =  MsPerFrame - (ticksEnd - ticksBegin);
		while (sleepTime < 0){
			sleepTime = sleepTime + MsPerFrame;
			//std::cout<<"*Frame Dropped*\n";
		}
		std::chrono::milliseconds dura(sleepTime);
		std::this_thread::sleep_for(dura);
	}	

	// Close Application Normally
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}

