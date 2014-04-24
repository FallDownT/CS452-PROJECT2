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
vec3 VelInitial(-0.1,-0.1,0.2);
float VelIncrementZ = 0.04;
float VelMaxZ = 1.2;

const int ms_per_frame = 50; //20fps, my vm runs really slow
vec3 ballVel(VelInitial.x,VelInitial.y,VelInitial.z);
int score = 0;

struct collisionInfo{
	bool isColliding;
	bool isComingFromPaddle;
	float location;	//location of collision on paddle

	collisionInfo() : isColliding(false), isComingFromPaddle(false), location(0.0) {}
	
} collision;

// Prototypes
void updateBallPosition(bool);

// Model and view matrices uniform location
GLuint  mMatrix, vMatrix, pMatrix;
GLuint vao1, vao2, ebo1, ebo2;
GLuint programP, programB;
mat4 modelP, modelB;

// Create camera view variables
point4 at( 0.0, 0.0, 0.0, 1.0 );
point4 eye( 0.0, 0.0, 8.0, 1.0 );
vec4   up( 0.0, 1.0, 0.0, 0.0 );

GLfloat positionArray[]={
	// Paddle
	-2.0,-2.0,0.0,
	-2.0,2.0,0.0,
	2.0,2.0,0.0,
	2.0,-2.0,0.0,

	// Ball
	-0.5,-0.5,0.0,
	-0.5,0.5,0.0,
	0.5,0.5,0.0,
	0.5,-0.5,0.0
};

GLfloat colorArray[]={
	// Paddle
	1.0,0.0,0.0,0.0,
	1.0,0.0,0.0,0.0,
	1.0,0.0,0.0,0.0,
	1.0,0.0,0.0,0.0,

	// Ball
	1.0f,1.0f,0.0f,1.0f,
	0.0f,0.0f,0.0f,1.0f,
	0.0f,1.0f,1.0f,1.0f,
	1.0f,0.0f,1.0f,1.0f
};
											
GLubyte elemsArray[]={
	0,1,2,3
};

// Define Constants
GLuint NumVerticies = 4;
GLuint BallHeight = 1;
GLuint BallWidth = 1;
GLuint PaddleHeight = 6;
GLuint PaddleWidth = 1;

//----------------------------------------------------------------------------

// OpenGL initialization
void init()
{
    // Load shaders and use the resulting shader program
    programP = InitShader( "vshaderP.glsl", "fshader.glsl" );
    programB = InitShader( "vshaderB.glsl", "fshader.glsl" );

	// Define data members
    GLuint vbo;
	size_t posDataOffset, colorDataOffset, paddleElemsArray, ballElemsSize;

	// --------------------------------------------------------------
	// -------  V E R T E X    A R R A Y    O B J E C T    1  -------
	// --------------------------------------------------------------
	// Define offsets and sizes
	posDataOffset = 0;
	colorDataOffset = sizeof(positionArray);

	// Use programP
    glUseProgram( programP );

    // Generate and bind new vertex array object
    glGenVertexArrays( 1,&vao1 );
    glBindVertexArray( vao1 );

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
	glGenBuffers( 1,&ebo1 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,ebo1 );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

	// Release bind to vao1 and programP
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// -------  V E R T E X    A R R A Y    O B J E C T    2  -------
	// --------------------------------------------------------------
	// Define new offsets
	posDataOffset += sizeof(GLfloat) * 3 * NumVerticies;
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;

	// Use programB
    glUseProgram( programB );

    // Generate new vertex array object
    glGenVertexArrays( 1,&vao2 );
    glBindVertexArray( vao2 );

	// Bind vertex buffer object
	// --Use same vbo as vao1 (no new buffer has been bound)
	
    // Bind attributes to vertex array
	glVertexAttribPointer(in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset));
    glEnableVertexAttribArray( in_position );

	glVertexAttribPointer(in_color,4,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(colorDataOffset));
    glEnableVertexAttribArray( in_color );

	// Generate and bind element buffer object
	glGenBuffers( 1,&ebo2 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,ebo2 );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

	// Release bind to vao2 and programB
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------

    // Retrieve transformation uniform variable locations
    mMatrix = glGetUniformLocation( programP, "modelMatrix" );
    vMatrix = glGetUniformLocation( programP, "viewMatrix" );
    pMatrix = glGetUniformLocation( programP, "projectionMatrix" );

	// Initialize model matrices to their correct positions
	modelP = modelB = identity();
	modelP = modelP * Translate(PaddlePosInitial);
    
    glEnable( GL_DEPTH_TEST );
    
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); // black background 
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
	collision.isColliding = false;
	collision.isComingFromPaddle = false;
	collision.location = 0.0;
	modelB = identity();
	ballVel = VelInitial;
	updateBallPosition(true);
}

//----------------------------------------------------------------------------

void display( SDL_Window* screen ){
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Define view
    mat4 view = LookAt( eye, at, up );

	// Draw elements of vao1
	glUseProgram( programP );
	glBindVertexArray( vao1 );
    glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelP );
    glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
    glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	// Draw elements of vao2
	glUseProgram( programB );
	glBindVertexArray( vao2 );
    glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelB );
    glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
    glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	// Release binds and swap buffers
	glBindVertexArray( 0 );
	glUseProgram( 0 );
    glFlush();
	SDL_GL_SwapWindow(screen);
}

//----------------------------------------------------------------------------

void input(SDL_Window* screen ){

	SDL_Event event;
	float paddleX = modelP[0][3];
	float paddleY = modelP[1][3];

	while (SDL_PollEvent(&event)){//Handling the keyboard
		switch (event.type){
		case SDL_QUIT:
			exit(0);
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym){
			case SDLK_ESCAPE:
				exit(0);
			case SDLK_w:	//paddle up
				if (paddleY < 7.0) {
					modelP = modelP * Translate(0.0,1.0,0.0);
				}
				//std::cout<<"modelP="<<std::endl;
				//printMat4(modelP);
				//std::cout<<std::endl;
				break;
			case SDLK_s:	//paddle down;
				if (paddleY > -7.0) {
					modelP = modelP * Translate(0.0,-1.0,0.0);
				}
				//std::cout<<"modelP="<<std::endl;
				//printMat4(modelP);
				//std::cout<<std::endl;
				break;
			case SDLK_d:	//paddle right;
				if (paddleX < 7.0) {
					modelP = modelP * Translate(1.0,0.0,0.0);
				}
				//std::cout<<"modelP="<<std::endl;
				//printMat4(modelP);
				//std::cout<<std::endl;
				break;
			case SDLK_a:	//paddle left;
				if (paddleX > -7.0) {
					modelP = modelP * Translate(-1.0,0.0,0.0);
				}
				//std::cout<<"modelP="<<std::endl;
				//printMat4(modelP);
				//std::cout<<std::endl;
				break;
			case SDLK_r://new game
				std::cout<<"*new game*"<<std::endl;
				score = 0;
				modelP = identity();
				modelP = modelP * Translate(0.0,0.0,-10.0);
				resetGame();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------

collisionInfo detectCollision(){
	// Get positions
	int ballLx, ballRx, paddleLx, paddleRx;
	ballLx = 10.0*(modelB[0][3] - BallWidth/2.0);
	ballRx = 10.0*(modelB[0][3] + BallWidth/2.0);
	paddleLx = 10.0*(modelP[0][3] - PaddleWidth/2.0);
	paddleRx = 10.0*(modelP[0][3] + PaddleWidth/2.0);

	float ballCy, paddleCy;
	ballCy = modelB[1][3];
	paddleCy = modelP[1][3];

	// Check for collision with paddle
	if (ballLx >= paddleLx && ballLx <= paddleRx){
		float heightOfImpact = ballCy - paddleCy;
		if (abs(heightOfImpact) < ((PaddleHeight+BallHeight)/2)){
			collision.isColliding=true;
			collision.isComingFromPaddle=true;
			collision.location=heightOfImpact;
		}
	} // No collision detected
	else {
		collision.isColliding=false;		
	}
	
	return collision;
}

//----------------------------------------------------------------------------

void updateScore(){
	int ballPositionX = 10.0*(modelB[0][3]);
	int leftWall = -130;
	int rightWall = 130;
	// Player 1 scores
	if (ballPositionX >= rightWall){
		score++;
		std::cout<<"Player scored!"<<std::endl;
		std::cout<<"Score is "<<score<<std::endl;
		resetGame();
	}
}

//----------------------------------------------------------------------------

void updateSpeed(){
	if (collision.isColliding && ballVel.z <= VelMaxZ){
		ballVel.z = ballVel.z + VelIncrementZ;
	}
}

//----------------------------------------------------------------------------

vec3 calculateTrajectory(){
	// Define data members
	vec3 t(0.0);
	int direction;

	if (collision.isComingFromPaddle){
		direction = 1;
	} else {
		direction = -1;
	}

	t.x = direction*ballVel.z;
	t.y = collision.location/8.0;

	return t;
}

//----------------------------------------------------------------------------

void updateBallPosition(bool forceUpdate){

	static vec3 t(VelInitial.x,VelInitial.y,VelInitial.z);
	
	if (forceUpdate){	// Reset to initial
		t = vec3(VelInitial.x,VelInitial.y,VelInitial.z);
	} else if (collision.isColliding){
		t = calculateTrajectory();
		modelB = modelB * Translate(t.x, t.y, t.z);
	} else {
		int ballPosY = 10.0*(modelB[1][3]);
		if ((ballPosY <= 100 && ballPosY >= 92) || (ballPosY >= -100 && ballPosY <= -92)){
			//hitting the ceiling or floor
			t.y = -t.y;
		}

		modelB = modelB * Translate(t.x, t.y, t.z);
	}
}

//----------------------------------------------------------------------------

void
reshape( int width, int height )
{
    glViewport( 0, 0, width, height );

    GLfloat left = -10.0, right = 10.0;
    GLfloat top = 10.0, bottom = -10.0;
    GLfloat zNear = -20.0, zFar = 20.0;

    GLfloat aspect = GLfloat(width)/height;

    if ( aspect > 1.0 ) {
		left *= aspect;
		right *= aspect;
    } else {
		top /= aspect;
		bottom /= aspect;
    }

    mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );

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
	int ticks_0, ticks_1;
	
	if(SDL_Init(SDL_INIT_VIDEO)<0){//initilizes the SDL video subsystem
		fprintf(stderr,"Unable to create window: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);//die on error
	}

	//create window
	window = SDL_CreateWindow(
		"Beamer's Crew - Project 2",	//Window title
		SDL_WINDOWPOS_UNDEFINED,	//initial x position
		SDL_WINDOWPOS_UNDEFINED,	//initial y position
		512,						//width, in pixels
		384,						//height, in pixels
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

	while (true) {
		ticks_0 = SDL_GetTicks();

		input(window);
		collision = detectCollision();
		updateScore();
		updateSpeed();		
		updateBallPosition(false);
		reshape(512,384);
		display(window);

		ticks_1 = SDL_GetTicks();
		sleepTime =  ms_per_frame - (ticks_1 - ticks_0);

		while (sleepTime < 0){
			sleepTime = sleepTime + ms_per_frame;
			//std::cout<<"*Frame Dropped*\n";
		}
			
		std::chrono::milliseconds dura(sleepTime);
		std::this_thread::sleep_for(dura);
	}

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
    return 0;
}

