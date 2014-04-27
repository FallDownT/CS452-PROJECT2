// fragment shading of sphere model

#include "Angel.h"
#include <iostream>
#include <cstdlib>
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include <chrono>
#include <thread>
#include <cmath>

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

// Constants
vec3 PaddlePosInitial(0.0,0.0,5.0);
vec3 WallPosInitial(0.0,0.0,-5.0);
vec3 BallPosInitial(0.0,0.0,0.0);
vec3 VelInitial(-0.1,-0.1,0.2);
float VelIncrementZ = 0.04;
float VelMaxZ = 1.2;
float LeftWallX10 = -100.0;
float RightWallX10 = 100.0;
float FloorY10 = -70.0;
float CeilingY10 = 70.0;
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

// Model and view matrices uniform location
GLuint  mMatrix, vMatrix, pMatrix;
GLuint vaoP, vaoW, vaoB, eboP, eboW, eboB;
GLuint programP, programW, programB;
mat4 modelP, modelW, modelB;

// Create camera view variables
point4 at( 0.0, 0.0, 0.0, 1.0 );
point4 eye( 0.0, 0.0, 7.0, 1.0 );
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
	10.0,-7.0,0.0,

	// Ball
	-0.5,-0.5,0.0,
	-0.5,0.5,0.0,
	0.5,0.5,0.0,
	0.5,-0.5,0.0
};

GLfloat colorArray[]={
	// Paddle
	1.0,0.0,0.0,0.5,
	1.0,0.0,0.0,0.5,
	1.0,0.0,0.0,0.5,
	1.0,0.0,0.0,0.5,

	// Wall
	0.0,0.0,1.0,0.0,
	0.0,0.0,1.0,0.0,
	0.0,0.0,1.0,0.0,
	0.0,0.0,1.0,0.0,

	// Ball
	1.0f,1.0f,0.0f,1.0f,
	0.0f,0.0f,0.0f,1.0f,
	0.0f,1.0f,1.0f,1.0f,
	1.0f,0.0f,1.0f,1.0f
};
											
GLubyte elemsArray[]={
	0,1,2,3
};

// Define geometric Constants
GLuint NumVerticies = 4;
GLfloat BallRadius = 0.5;
GLuint PaddleHeight = 4;
GLuint PaddleWidth = 4;

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

// OpenGL initialization
void init()
{
    // Load shaders and use the resulting shader program
    programP = InitShader( "vshaderP.glsl", "fshader.glsl" );
    programW = InitShader( "vshaderW.glsl", "fshader.glsl" );
    programB = InitShader( "vshaderB.glsl", "fshader.glsl" );

	// Define data members
    GLuint vbo;
	size_t posDataOffset, colorDataOffset;

	// ------------------------------------------------------------------------
	// -------  V E R T E X    A R R A Y    O B J E C T    P A D D L E  -------
	// ------------------------------------------------------------------------
	// Define offsets and sizes
	posDataOffset = 0;
	colorDataOffset = sizeof(positionArray);

	// Use programP
    glUseProgram( programP );

    // Generate and bind new vertex array object
    glGenVertexArrays( 1,&vaoP );
    glBindVertexArray( vaoP );

	// Generate and bind new vertex buffer object and populate the buffer
	glGenBuffers( 1,&vbo );
	glBindBuffer( GL_ARRAY_BUFFER,vbo );
	glBufferData( GL_ARRAY_BUFFER,sizeof(positionArray) + sizeof(colorArray),NULL,GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER,posDataOffset,sizeof(positionArray),positionArray );
	glBufferSubData( GL_ARRAY_BUFFER,colorDataOffset,sizeof(colorArray),colorArray );

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
	// -------  V E R T E X    A R R A Y    O B J E C T    W A L L  -------
	// --------------------------------------------------------------------
	// Define new offsets
	posDataOffset += sizeof(GLfloat) * 3 * NumVerticies;
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;

	// Use programW
    glUseProgram( programW );

    // Generate new vertex array object
    glGenVertexArrays( 1,&vaoW );
    glBindVertexArray( vaoW );

	// Bind vertex buffer object
	// --Use same vbo as vaoP (no new buffer has been bound)
	
    // Bind attributes to vertex array
	glVertexAttribPointer(in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset));
    glEnableVertexAttribArray( in_position );

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
	// -------  V E R T E X    A R R A Y    O B J E C T    B A L L  -------
	// --------------------------------------------------------------------
	// Define new offsets
	posDataOffset += sizeof(GLfloat) * 3 * NumVerticies;
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;

	// Use programB
    glUseProgram( programB );

    // Generate new vertex array object
    glGenVertexArrays( 1,&vaoB );
    glBindVertexArray( vaoB );

	// Bind vertex buffer object
	// --Use same vbo as vaoP (no new buffer has been bound)
	
    // Bind attributes to vertex array
	glVertexAttribPointer(in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset));
    glEnableVertexAttribArray( in_position );

	glVertexAttribPointer(in_color,4,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(colorDataOffset));
    glEnableVertexAttribArray( in_color );

	// Generate and bind element buffer object
	glGenBuffers( 1,&eboB );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,eboB );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

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
	glDisable(GL_CULL_FACE);

	

    
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); // black background
//	glClearColor( 1.0, 1.0, 1.0, 1.0 ); // black background 
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
	modelP = identity() * Translate(PaddlePosInitial);
	collision.isColliding = false;
	collision.isComingFromPaddle = false;
	collision.locationX = 0.0;
	collision.locationY = 0.0;
	modelB = identity();
	ballVel = VelInitial;
	score = 0;
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
    glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);
	
	// Draw elements of vaoP
	glUseProgram( programP );
	glBindVertexArray( vaoP );
    glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelP );
    glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
    glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	// Draw elements of vaoW
	glUseProgram( programW );
	glBindVertexArray( vaoW );
    glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelW );
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
			case SDLK_w:	//paddle up
				if (modelP[1][3] < 6.5) {
					modelP = modelP * Translate(0.0,1.0,0.0);
				}
				break;
			case SDLK_s:	//paddle down;
				if (modelP[1][3] > -6.5) {
					modelP = modelP * Translate(0.0,-1.0,0.0);
				}
				break;
			case SDLK_d:	//paddle right;
				if (modelP[0][3] < 9.5) {
					modelP = modelP * Translate(1.0,0.0,0.0);
				}
				break;
			case SDLK_a:	//paddle left;
				if (modelP[0][3] > -9.5) {
					modelP = modelP * Translate(-1.0,0.0,0.0);
				}
				break;
			case SDLK_r://new game
				resetGame();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------

void updateCollision(){
	// Get positions
	int paddleLx, paddleRx, paddleBy, paddleTy, paddleFz, paddleNz;
	int ballLx, ballRx, ballBy, ballTy, ballFz, ballNz;
	paddleLx = 10.0*(modelP[0][3] - PaddleWidth/2.0);
	paddleRx = 10.0*(modelP[0][3] + PaddleWidth/2.0);
	paddleBy = 10.0*(modelP[1][3] - PaddleHeight/2.0);
	paddleTy = 10.0*(modelP[1][3] + PaddleHeight/2.0);
	paddleFz = 10.0*(modelP[2][3]);
	paddleNz = 10.0*(modelP[2][3]);
	ballLx = 10.0*(modelB[0][3] - BallRadius);
	ballRx = 10.0*(modelB[0][3] + BallRadius);
	ballBy = 10.0*(modelB[1][3] - BallRadius);
	ballTy = 10.0*(modelB[1][3] + BallRadius);
	ballFz = 10.0*(modelB[2][3] - BallRadius);
	ballNz = 10.0*(modelB[2][3] + BallRadius);

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
	if ((ballLx <= LeftWallX10 && ballVel.x < 0) ||
		(ballRx >= RightWallX10 && ballVel.x > 0)){

		//std::cout<<"Collision with left/right wall"<<std::endl;
		ballVel.x = -ballVel.x;
	}
	
	// Check for floor/ceiling collision 
	if ((ballBy <= FloorY10 && ballVel.y < 0) ||
		(ballTy >= CeilingY10 && ballVel.y > 0)){

		//std::cout<<"Collision with floor/ceiling"<<std::endl;
		//printMat4(modelB);
		ballVel.y = -ballVel.y;
	}
}

//----------------------------------------------------------------------------

void updateScore(){
	int ballPositionZ = 10.0*(modelB[2][3]);
	int backWall = 60.0;
	
	// Player hit the ball
	if (collision.isColliding && collision.isComingFromPaddle){
		score++;
		std::cout<<"Score: "<<score<<std::endl;
	}
	// Player loses
	if (ballPositionZ >= backWall){
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

    GLfloat left = -10.0, right = 10.0;
    GLfloat top = 10.0, bottom = -10.0;
    GLfloat zNear = -10.0, zFar = 10.0;

    GLfloat aspect = GLfloat(width)/height;

    if ( aspect > 1.0 ) {
		left *= aspect;
		right *= aspect;
    } else {
		top /= aspect;
		bottom /= aspect;
    }

    mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );
	//mat4 projection = Frustum( left, right, bottom, top, zNear, zFar );

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

