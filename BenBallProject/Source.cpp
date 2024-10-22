#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

using namespace std;

struct Ball
{
	float px, py;
	float vx, vy;
	float ax, ay;
	float ox, oy;
	float radius;
	float mass;
	float friction;
	int score;
	int id;
	float fSimTimeRemaining;
	olc::Pixel col;
};

struct sLineSegment
{
	float sx, sy;
	float ex, ey;
	float radius;
};


class Engine : public olc::PixelGameEngine
{
public:
	Engine()
	{
		sAppName = "Ball project";
	}
private:
	vector<Ball> vecBalls;
	vector<sLineSegment> vecLines;
	vector<pair<float, float>> modelCircle;
	Ball* pSelectedBall = nullptr;
	olc::Sprite *spriteBalls = nullptr;
	sLineSegment* pSelectedLine = nullptr;
	bool bSelectedLineStart = false;
	int relax;
	static float volume;
	
	void AddBall(float x, float y, float r = 5.0f, int s = 0)
	{
		Ball b;
		b.px = x; b.py = y;
		b.vx = 0; b.vy = 0;
		b.ax = 0; b.ay = 0;
		b.ox = 0; b.oy = 0;
		b.radius = r;
		b.mass = r * 10.0f;
		b.friction = 0.0f;
		b.score = s;
		b.fSimTimeRemaining = 0.0f;
		b.id = vecBalls.size();
		b.col = olc::Pixel(rand() % 200 + 55, rand() % 200 + 55, rand() % 200 + 55);
		vecBalls.emplace_back(b);
	}

	void ClearBalls()
	{
		vecBalls.clear();
	}

	olc::Sprite spr;

private:
	static float MyCustomFilterFunction(int nChannel, float fGlobalTime, float fSample)
	{
		// Control the volume
		float fOutput = fSample * volume;

		return fOutput;
	}

public:
	bool OnUserCreate()
	{
		olc::SOUND::InitialiseAudio(44100, 1, 8, 512);

		relax = olc::SOUND::LoadAudioSample("bensound-slowmotion.wav");

		float fLineRadius = 5.0f;
		vecLines.push_back({0, 0, (float)ScreenWidth(), 0, fLineRadius});
		vecLines.push_back({0, (float)ScreenHeight(), (float)ScreenWidth(), (float)ScreenHeight(), fLineRadius});
		vecLines.push_back({0, 0, 0, (float)ScreenHeight(), fLineRadius});
		vecLines.push_back({ (float)ScreenWidth(), 0, (float)ScreenWidth(), (float)ScreenHeight(), fLineRadius });
		olc::SOUND::PlaySample(relax, true);

		olc::SOUND::SetUserFilterFunction(MyCustomFilterFunction);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime)
	{
		
		float fBallRadius = 5.0f;
		

		auto DoCirclesOverlap = [](float x1, float y1, float r1, float x2, float y2, float r2)
		{
			return fabs((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2)) <= ((r1 + r2) * (r1 + r2));
		};

		auto IsPointInCircle = [](float x1, float y1, float r1, float px, float py)
		{
			return fabs((x1 - px)*(x1 - px) + (y1 - py)*(y1 - py)) < (r1 * r1);
		};

		//Handle Keys

		if (GetKey(olc::SPACE).bPressed)
		{
			cout << "Ball Added" << endl;
			AddBall(((float)rand() / (float)RAND_MAX) * ScreenWidth(), ((float)rand() / (float)RAND_MAX) * ScreenHeight(), fBallRadius); 
		}
		if (GetKey(olc::DEL).bPressed)
		{
			ClearBalls();
		}

		if (GetKey(olc::O).bPressed)
		{
			volume -= 3.0f * fElapsedTime;
			std::cout << "Volume Decreased" << std::endl;
		}

		if (GetKey(olc::P).bPressed)
		{
			volume += 3.0f * fElapsedTime;
			std::cout << "Volume Increased" << std::endl;
		}

		if (volume < 0.0f) volume = 0.0f;
		if (volume > 1.0f) volume = 1.0f;



		if (GetMouse(0).bHeld)
		{
			for (auto &ball : vecBalls)
			{
				ball.vx += (GetMouseX() - ball.px) * 0.3f;
				ball.vy += (GetMouseY() - ball.py) * 0.3f;
			}
		}
		if (GetMouse(0).bReleased)
		{
			for (auto& ball : vecBalls)
			{
				ball.vx += (GetMouseX() + ball.px) * 0.2f;
				ball.vy += (GetMouseY() + ball.py) * 0.2f;
			}
		}



		vector<pair<Ball*, Ball*>> vecCollidingPairs;
		vector<Ball*> vecFakeBalls;

		// Threshold indicating stability of object
		float fStable = 0.005f;

		// Multiple simulation updates with small time steps permit more accurate physics
		// and realistic results at the expense of CPU time of course
		int nSimulationUpdates = 4;

		// Multiple collision trees require more steps to resolve. Normally we would
		// continue simulation until the object has no simulation time left for this
		// epoch, however this is risky as the system may never find stability, so we
		// can clamp it here
		int nMaxSimulationSteps = 15;

		// Break up the frame elapsed time into smaller deltas for each simulation update
		float fSimElapsedTime = fElapsedTime / (float)nSimulationUpdates;

		// Main simulation loop
		for (int i = 0; i < nSimulationUpdates; i++)
		{
			// Set all balls time to maximum for this epoch
			for (auto &ball : vecBalls)
				ball.fSimTimeRemaining = fSimElapsedTime;

			// Erode simulation time on a per object basis, depending upon what happens
			// to it during its journey through this epoch
			for (int j = 0; j < nMaxSimulationSteps; j++)
			{
				// Update Ball Positions
				for (auto &ball : vecBalls)
				{
					if (ball.fSimTimeRemaining > 0.0f)
					{
						ball.ox = ball.px;								// Store original position this epoch
						ball.oy = ball.py;

						ball.ax = -ball.vx * 0.5f;						// Apply drag and gravity
						ball.ay = -ball.vy * 0.1f + 200.0f;
						

						ball.vx += ball.ax * ball.fSimTimeRemaining;	// Update Velocity
						ball.vy += ball.ay * ball.fSimTimeRemaining;

						ball.px += ball.vx * ball.fSimTimeRemaining;	// Update position
						ball.py += ball.vy * ball.fSimTimeRemaining;

						
					
						// Stop ball when velocity is neglible
						if (fabs(ball.vx*ball.vx + ball.vy*ball.vy) < fStable)
						{
							ball.vx = 0;
							ball.vy = 0;
						}
					}
				}


				// Work out static collisions with walls and displace balls so no overlaps
				for (auto &ball : vecBalls)
				{
					float fDeltaTime = ball.fSimTimeRemaining;

					// Against Edges
					for (auto &edge : vecLines)
					{
						// Check that line formed by velocity vector, intersects with line segment
						float fLineX1 = edge.ex - edge.sx;
						float fLineY1 = edge.ey - edge.sy;

						float fLineX2 = ball.px - edge.sx;
						float fLineY2 = ball.py - edge.sy;

						float fEdgeLength = fLineX1 * fLineX1 + fLineY1 * fLineY1;

						// This is nifty - It uses the DP of the line segment vs the line to the object, to work out
						// how much of the segment is in the "shadow" of the object vector. The min and max clamp
						// this to lie between 0 and the line segment length, which is then normalised. We can
						// use this to calculate the closest point on the line segment
						float t = max(0.0f, min(fEdgeLength, (fLineX1 * fLineX2 + fLineY1 * fLineY2))) / fEdgeLength;

						// Which we do here
						float fClosestPointX = edge.sx + t * fLineX1;
						float fClosestPointY = edge.sy + t * fLineY1;

						// And once we know the closest point, we can check if the ball has collided with the segment in the
						// same way we check if two balls have collided
						float fDistance = sqrtf((ball.px - fClosestPointX)*(ball.px - fClosestPointX) + (ball.py - fClosestPointY)*(ball.py - fClosestPointY));

						if (fDistance <= (ball.radius + edge.radius))
						{
							// Collision has occurred - treat collision point as a ball that cannot move. To make this
							// compatible with the dynamic resolution code below, we add a fake ball with an infinite mass
							// so it behaves like a solid object when the momentum calculations are performed
							Ball *fakeball = new Ball();
							fakeball->radius = edge.radius;
							fakeball->mass = ball.mass * 0.8f;
							fakeball->px = fClosestPointX;
							fakeball->py = fClosestPointY;
							fakeball->vx = -ball.vx;	// We will use these later to allow the lines to impart energy into ball
							fakeball->vy = -ball.vy;	// if the lines are moving, i.e. like pinball flippers

							// Store Fake Ball
							vecFakeBalls.push_back(fakeball);

							// Add collision to vector of collisions for dynamic resolution
							vecCollidingPairs.push_back({ &ball, fakeball });

							// Calculate displacement required
							float fOverlap = 1.0f * (fDistance - ball.radius - fakeball->radius);

							// Displace Current Ball away from collision
							ball.px -= fOverlap * (ball.px - fakeball->px) / fDistance;
							ball.py -= fOverlap * (ball.py - fakeball->py) / fDistance;
						}
					}

					// Against other balls
					for (auto &target : vecBalls)
					{
						if (ball.id != target.id) // Do not check against self
						{
							if (DoCirclesOverlap(ball.px, ball.py, ball.radius, target.px, target.py, target.radius))
							{
								// Collision has occured
								vecCollidingPairs.push_back({ &ball, &target });

								// Distance between ball centers
								float fDistance = sqrtf((ball.px - target.px)*(ball.px - target.px) + (ball.py - target.py)*(ball.py - target.py));

								// Calculate displacement required
								float fOverlap = 0.7f * (fDistance - ball.radius - target.radius);

								// Displace Current Ball away from collision
								ball.px -= fOverlap * (ball.px - target.px) / fDistance;
								ball.py -= fOverlap * (ball.py - target.py) / fDistance;

								// Displace Target Ball away from collision - Note, this should affect the timing of the target ball
								// and it does, but this is absorbed by the target ball calculating its own time delta later on
								target.px += fOverlap * (ball.px - target.px) / fDistance;
								target.py += fOverlap * (ball.py - target.py) / fDistance;
							}
						}
					}

					// Time displacement - we knew the velocity of the ball, so we can estimate the distance it should have covered
					// however due to collisions it could not do the full distance, so we look at the actual distance to the collision
					// point and calculate how much time that journey would have taken using the speed of the object. Therefore
					// we can now work out how much time remains in that timestep.
					float fIntendedSpeed = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
					float fIntendedDistance = fIntendedSpeed * ball.fSimTimeRemaining;
					float fActualDistance = sqrtf((ball.px - ball.ox)*(ball.px - ball.ox) + (ball.py - ball.oy)*(ball.py - ball.oy));
					float fActualTime = fActualDistance / fIntendedSpeed;

					// After static resolution, there may be some time still left for this epoch, so allow simulation to continue
					ball.fSimTimeRemaining = ball.fSimTimeRemaining - fActualTime;
				}

				// Now work out dynamic collisions
				float fEfficiency = 1.00f;
				for (auto c : vecCollidingPairs)
				{
					Ball *b1 = c.first, *b2 = c.second;

					// Distance between balls
					float fDistance = sqrtf((b1->px - b2->px)*(b1->px - b2->px) + (b1->py - b2->py)*(b1->py - b2->py));

					// Normal
					float nx = (b2->px - b1->px) / fDistance;
					float ny = (b2->py - b1->py) / fDistance;

					// Tangent
					float tx = -ny;
					float ty = nx;

					// Dot Product Tangent
					float dpTan1 = b1->vx * tx + b1->vy * ty;
					float dpTan2 = b2->vx * tx + b2->vy * ty;

					// Dot Product Normal
					float dpNorm1 = b1->vx * nx + b1->vy * ny;
					float dpNorm2 = b2->vx * nx + b2->vy * ny;

					// Conservation of momentum in 1D
					float m1 = fEfficiency * (dpNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dpNorm2) / (b1->mass + b2->mass);
					float m2 = fEfficiency * (dpNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dpNorm1) / (b1->mass + b2->mass);

					// Update ball velocities
					b1->vx = tx * dpTan1 + nx * m1;
					b1->vy = ty * dpTan1 + ny * m1;
					b2->vx = tx * dpTan2 + nx * m2;
					b2->vy = ty * dpTan2 + ny * m2;
				}

				// Remove collisions
				vecCollidingPairs.clear();

				// Remove fake balls
				for (auto &b : vecFakeBalls) delete b;
				vecFakeBalls.clear();
			}
		}

		// Clear the Screen
		FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::Pixel(0, 0, 0));

		// Draw lines
		for (auto line : vecLines)
		{
			FillCircle(line.sx, line.sy, line.radius, olc::Pixel(255, 255, 255));
			FillCircle(line.ex, line.ey, line.radius, olc::Pixel(128, 128, 128));

			float nx = -(line.ey - line.sy);
			float ny = (line.ex - line.sx);
			float d = sqrt(nx*nx + ny * ny);
			nx /= d;
			ny /= d;

			DrawLine((line.sx + nx * line.radius), (line.sy + ny * line.radius), (line.ex + nx * line.radius), (line.ey + ny * line.radius), olc::Pixel(255, 255, 255));
			DrawLine((line.sx - nx * line.radius), (line.sy - ny * line.radius), (line.ex - nx * line.radius), (line.ey - ny * line.radius), olc::Pixel(255, 255, 255));
		}

		// Draw Balls
		for (auto ball : vecBalls)
		{
			ball.col = olc::Pixel(rand() % 200 + 55, rand() % 200 + 55, rand() % 200 + 55);
			FillCircle(ball.px, ball.py, ball.radius, ball.col);
			
		}

		// Draw Cue
		if (pSelectedBall != nullptr)
			DrawLine(pSelectedBall->px, pSelectedBall->py, GetMouseX(), GetMouseY(), olc::Pixel(0, 0, 255));




		return true;
	}

	bool OnUserDestroy()
	{
		olc::SOUND::DestroyAudio();
		return true;
	}

};

float Engine::volume = 1.0f;

int main()
{
	Engine engine;

	int width = 1300;
	int height = width * 9 / 16;

	if (engine.Construct(width, height, 1, 1))
	{
		engine.Start();
	}
	else
		std::cout << "Could not construct console" << std::endl;

	return 0;
}