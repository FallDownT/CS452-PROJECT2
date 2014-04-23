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
float SpeedFactorInitial = 0.2;
float SpeedFactorIncrement = 0.04;
float SpeedFactorMax = 1.2;
float BallTrajectoryInitial = -0.1;

const int ms_per_frame = 50; //20fps, my vm runs really slow
float speedFactor = SpeedFactorInitial;
float ballTrajectory = BallTrajectoryInitial; //slope of ball's path, factor of aspect ratio
int score[2] = {0,0}; //element 0 is player 1's score, element 1 is player 2's score

struct collisionInfo{
	bool isColliding;	//true if collision, false if no collision
	bool isComingFromPlayer1;	//true if player 1, false if player 2
	float location;	//location of the collision on the paddle. determines return path

	collisionInfo() : isColliding(false), isComingFromPlayer1(false), location(0.0) {}
	
} collision;


void updateBallPosition(bool);

// Model and view matrices uniform location
GLuint  mMatrix, vMatrix, pMatrix;

// Define global vaos and ebos
GLuint vao1, vao2, vao3, ebo1, ebo2, ebo3;

// Define programs
GLuint programP1, programP2, programB;

// Define model matrices
mat4 modelP1, modelP2, modelB;


// Create camera view variables
point4 at( 0.0, 0.0, 0.0, 1.0 );
point4 eye( 0.0, 0.0, 5.0, 1.0 );
vec4   up( 0.0, 10.0, 0.0, 0.0 );

GLfloat positionArray[]={
	// Paddle
	-0.5,-3.0,0.0,
	-0.5,3.0,0.0,
	0.5,3.0,0.0,
	0.5,-3.0,0.0,

	// Ball
	-0.5,-0.5,0.0,
	-0.5,0.5,0.0,
	0.5,0.5,0.0,
	0.5,-0.5,0.0
};

GLfloat colorArray[]={
	// Paddle 1
	1.0,0.0,0.0,0.0,
	1.0,0.0,0.0,0.0,
	1.0,0.0,0.0,0.0,
	1.0,0.0,0.0,0.0,

	// Paddle 2
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
    programP1 = InitShader( "vshaderP1.glsl", "fshader.glsl" );
    programP2 = InitShader( "vshaderP2.glsl", "fshader.glsl" );
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

	// Use programP1
    glUseProgram( programP1 );

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
	GLuint in_position = glGetAttribLocation( programP1, "in_position" );
	glVertexAttribPointer( in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset) );
    glEnableVertexAttribArray( in_position );

	// Bind color attribute of vbo
	GLuint in_color = glGetAttribLocation( programP1, "in_color" );
	glVertexAttribPointer( in_color,4,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(colorDataOffset) );
    glEnableVertexAttribArray( in_color );

	// Generate and bind element buffer object
	glGenBuffers( 1,&ebo1 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,ebo1 );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

	// Release bind to vao1 and programP1
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// -------  V E R T E X    A R R A Y    O B J E C T    2  -------
	// --------------------------------------------------------------
	// Define new offsets
	posDataOffset += 0;	// Duplicating last paddle
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;	// Different color

	// Use programP2
    glUseProgram( programP2 );

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

	// Release bind to vao2 and programP2
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------

	// --------------------------------------------------------------
	// -------  V E R T E X    A R R A Y    O B J E C T    3  -------
	// --------------------------------------------------------------
	// Define new offsets
	posDataOffset += sizeof(GLfloat) * 3 * NumVerticies;
	colorDataOffset += sizeof(GLfloat) * 4 * NumVerticies;

	// Use programB
    glUseProgram( programB );

    // Generate new vertex array object
    glGenVertexArrays( 1,&vao3 );
    glBindVertexArray( vao3 );

	// Bind vertex buffer object
	// --Use same vbo as vao1 (no new buffer has been bound)
	
    // Bind attributes to vertex array
	glVertexAttribPointer(in_position,3,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(posDataOffset));
    glEnableVertexAttribArray( in_position );

	glVertexAttribPointer(in_color,4,GL_FLOAT,GL_FALSE,0,BUFFER_OFFSET(colorDataOffset));
    glEnableVertexAttribArray( in_color );

	// Generate and bind element buffer object
	glGenBuffers( 1,&ebo3 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,ebo3 );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof(elemsArray),elemsArray,GL_STATIC_DRAW );

	// Release bind to vao3 and programB
	glBindVertexArray( 0 );
	glUseProgram( 0 );
	// --------------------------------------------------------------

    // Retrieve transformation uniform variable locations
    mMatrix = glGetUniformLocation( programP1, "modelMatrix" );
    vMatrix = glGetUniformLocation( programP1, "viewMatrix" );
    pMatrix = glGetUniformLocation( programP1, "projectionMatrix" );

	// Initialize model matrices to their correct positions
	modelP1 = modelP2 = modelB = identity();
	modelP1 = modelP1 * Translate(-10.0,0.0,0.0);
	modelP2 = modelP2 * Translate(10.0,0.0,0.0);
    
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
	speedFactor=SpeedFactorInitial;
	modelB = identity();
	collision.isColliding = false;
	collision.isComingFromPlayer1 = false;
	collision.location = 0.0;
	ballTrajectory = BallTrajectoryInitial;
	updateBallPosition(true);
}

//----------------------------------------------------------------------------

void display( SDL_Window* screen ){
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Define view
    mat4 view = LookAt( eye, at, up );

	// Draw elements of vao1
	glUseProgram( programP1 );
	glBindVertexArray( vao1 );
    glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelP1 );
    glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
    glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	// Draw elements of vao2
	glUseProgram( programP2 );
	glBindVertexArray( vao2 );
    glUniformMatrix4fv( mMatrix, 1, GL_TRUE, modelP2 );
    glUniformMatrix4fv( vMatrix, 1, GL_TRUE, view );
    glDrawElements( GL_TRIANGLE_FAN,sizeof(elemsArray),GL_UNSIGNED_BYTE,0 );
	glBindVertexArray( 0 );
	glUseProgram( 0 );

	// Draw elements of vao3
	glUseProgram( programB );
	glBindVertexArray( vao3 );
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

	while (SDL_PollEvent(&event)){//Handling the keyboard
		switch (event.type){
		case SDL_QUIT:exit(0);break;
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym){
			case SDLK_ESCAPE:exit(0);
			case SDLK_w://paddle 1 up;
				if (modelP1[1][3] < 7.0) {
					modelP1 = modelP1 * Translate(0.0,1.0,0.0);
				}
				//std::cout<<"modelP1="<<std::endl;
				//printMat4(modelP1);
				//std::cout<<std::endl;
				break;
			case SDLK_s://paddle 1 down;
				if (modelP1[1][3] > -7.0) {
					modelP1 = modelP1 * Translate(0.0,-1.0,0.0);
				}
				//std::cout<<"modelP1="<<std::endl;
				//printMat4(modelP1);
				//std::cout<<std::endl;
				break;
			case SDLK_i://paddle 2 up
				if (modelP2[1][3] < 7.0) {
					modelP2 = modelP2 * Translate(0.0,1.0,0.0);
				}
				break;
			case SDLK_k://paddle 2 down
				if (modelP2[1][3] > -7.0) {
					modelP2 = modelP2 * Translate(0.0,-1.0,0.0);
				}
			break;
			case SDLK_r://new game
				std::cout<<"*new game*\n";
				score[0]=0;
				score[1]=0;
				modelP1 = modelP2 = identity();
				modelP1 = modelP1 * Translate(-10.0,0.0,0.0);
				modelP2 = modelP2 * Translate(10.0,0.0,0.0);
				resetGame();
				break;
			}
		}
	}
}

//----------------------------------------------------------------------------

collisionInfo detectCollision(){
	// Get positions
	int ballLx, ballRx, p1Lx, p1Rx, p2Lx, p2Rx;
	ballLx = 10.0*(modelB[0][3] - BallWidth/2.0);
	ballRx = 10.0*(modelB[0][3] + BallWidth/2.0);
	p1Lx = 10.0*(modelP1[0][3] - PaddleWidth/2.0);
	p1Rx = 10.0*(modelP1[0][3] + PaddleWidth/2.0);
	p2Lx = 10.0*(modelP2[0][3] - PaddleWidth/2.0);
	p2Rx = 10.0*(modelP2[0][3] + PaddleWidth/2.0);

	float ballCy, p1Cy, p2Cy;
	ballCy = modelB[1][3];
	p1Cy = modelP1[1][3];
	p2Cy = modelP2[1][3];

	// Check for collision with paddle 1
	if (ballLx >= p1Lx && ballLx <= p1Rx){
		float heightOfImpact = ballCy - p1Cy;
		if (abs(heightOfImpact) < ((PaddleHeight+BallHeight)/2)){
			collision.isColliding=true;
			collision.isComingFromPlayer1=true;
			collision.location=heightOfImpact;
		}
	} // Check for collision with paddle 2
	else if (ballRx >= p2Lx && ballRx <= p2Rx){
		float heightOfImpact = ballCy - p2Cy;
		if (abs(heightOfImpact) < ((PaddleHeight+BallHeight)/2)){
			collision.isColliding=true;
			collision.isComingFromPlayer1=false;
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
		score[0]++;
		std::cout<<"Player 1 scored!\n";
		std::cout<<"Score is "<<score[0]<<" : "<<score[1]<<"\n\n";
		resetGame();
	} // Player 2 scores
	else if (ballPositionX <= leftWall){
		score[1]++;
		std::cout<<"Player 2 scored!\n";
		std::cout<<"Score is "<<score[0]<<" : "<<score[1]<<"\n\n";
		resetGame();
	}		
}

//----------------------------------------------------------------------------

void updateSpeed(){
	if (collision.isColliding && speedFactor <= SpeedFactorMax){
		speedFactor = speedFactor + SpeedFactorIncrement;
	}
}

//----------------------------------------------------------------------------

vec3 calculateTrajectory(){
	// Define data members
	vec3 t(0.0);
	int direction;

	if (collision.isComingFromPlayer1){
		direction = 1;
	} else {
		direction = -1;
	}

	t.x = direction*speedFactor;
	t.y = collision.location/8.0;

	return t;
}

//----------------------------------------------------------------------------

void updateBallPosition(bool forceUpdate){

	static vec3 t(-speedFactor,BallTrajectoryInitial,0.0);
	
	if (forceUpdate){
		t = vec3(-speedFactor, BallTrajectoryInitial, 0.0);
	} else if (collision.isColliding){
		t = calculateTrajectory();
		modelB = modelB * Translate(t.x, t.y, t.z);
	} else {
		int temp = 10.0*(modelB[1][3]);
		if ((temp <= 100 && temp >= 92) || (temp >= -100 && temp <= -92)){
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
	glUseProgram( programP1 );
    glUniformMatrix4fv( pMatrix, 1, GL_TRUE, projection );

	glUseProgram( programP2 );
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
		"Beamer's Crew - Project 1",	//Window title
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

