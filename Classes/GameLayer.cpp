#include "GameLayer.h"
#include "audio/include/AudioEngine.h"

void GameLayer::playerScore(int player)
{
	//AudioEngine::getInstance()->playEffect("score.wav");
	playScoreSfx();

	_ball->setVector(Vec2(0, 0));

	//Update score
	char score_buffer[10];
	if (player == 1) {
		_player1Score++;
		_player1ScoreLabel->setString(std::to_string(_player1Score));
		_ball->setNextPosition(Vec2(_screenSize.width * 0.5f, _screenSize.height * 0.5f + 2 * _ball->radius()));
	}
	else {
		_player2Score++;
		_player2ScoreLabel->setString(std::to_string(_player2Score));
		_ball->setNextPosition(Vec2(_screenSize.width * 0.5f, _screenSize.height * 0.5f - 2 * _ball->radius()));
	}

	//The players are moved to their original position
	_player1->setPosition(Vec2(_screenSize.width * 0.5,
		_player1->radius() * 2));
	_player2->setPosition(Vec2(_screenSize.width * 0.5,
		_screenSize.height - _player1->radius() * 2));
	_player1->setTouch(nullptr);
	_player2->setTouch(nullptr);

}

bool GameLayer::init()
{
	if (!Layer::init())
		return false;

	_audioID=AudioEngine::INVALID_AUDIO_ID;
	AudioEngine::preload("sfx/sfx_puck.mp3");
	AudioEngine::preload("sfx/sfx_score.mp3");

	_players = Vector<GameSprite*>(2);
	_player1Score = 0;
	_player2Score = 0;
	//_screenSize = _director->getWinSize();
	_screenSize = _director->getVisibleSize();
	Vec2 org=_director->getVisibleOrigin();
	scheduleUpdate();

	//Add white bg
	auto bg=LayerColor::create(Color4B::WHITE);
	addChild(bg,-1);
	//Add hockey ground
	{
		auto court = Sprite::create("hockey_ground.png");
		court->setPosition(Vec2(_screenSize.width * 0.5 + org.x, _screenSize.height * 0.5 + org.y));
		addChild(court);
		float scaleY = _screenSize.height / 1595;
		court->setScaleY(scaleY);
	}
	//Add player1
	_player1 = GameSprite::gameSpriteWithFile("hockey_striker.png");
	_player1->setPosition(Vec2(_screenSize.width * 0.5, _player1->radius() * 2));
	_players.pushBack(_player1);
	addChild(_player1);
	//Add player2
	_player2 = GameSprite::gameSpriteWithFile("hockey_striker.png");
	_player2->setPosition(Vec2(_screenSize.width * 0.5, _screenSize.height - _player1->radius() * 2));
	_players.pushBack(_player2);
	addChild(_player2);
	//Add puck
	_ball = GameSprite::gameSpriteWithFile("hockey_puck.png");
	_ball->setPosition(Vec2(_screenSize.width * 0.5f, _screenSize.height * 0.5f - 2 * _ball->radius()));
	addChild(_ball);

	//Add label1
	_player1ScoreLabel = Label::createWithTTF("0", "fonts/arial.ttf", 60);
	_player1ScoreLabel->setPosition(Vec2(_screenSize.width - 60, _screenSize.height * 0.5 - 80));
	_player1ScoreLabel->setRotation(90);
	_player1ScoreLabel->setTextColor(Color4B::BLACK);
	addChild(_player1ScoreLabel);
	//Add label2
	_player2ScoreLabel = Label::createWithTTF("0", "fonts/arial.ttf", 60);
	_player2ScoreLabel->setPosition(Vec2(_screenSize.width - 60, _screenSize.height * 0.5 + 80));
	_player2ScoreLabel->setRotation(90);
	_player2ScoreLabel->setTextColor(Color4B::BLACK);
	addChild(_player2ScoreLabel);

	//Add multitouch listener
	auto listener = EventListenerTouchAllAtOnce::create();
	listener->onTouchesBegan = CC_CALLBACK_2(GameLayer::onTouchesBegan, this);
	listener->onTouchesMoved = CC_CALLBACK_2(GameLayer::onTouchesMoved, this);
	listener->onTouchesEnded = CC_CALLBACK_2(GameLayer::onTouchesEnded, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
	
	return true;
}

void GameLayer::update(float dt)
{
	auto ballNextPosition = _ball->getNextPosition();
	auto ballVector = _ball->getVector();
	ballVector *= 0.98f;
	ballNextPosition.x += ballVector.x;
	ballNextPosition.y += ballVector.y;

	//Check for collisions in players and puck.
	float squared_radii = pow(_player1->radius() + _ball->radius(), 2);
	for (auto player : _players) {
		auto playerNextPosition = player->getNextPosition();
		auto playerVector = player->getVector();
		float diffx = ballNextPosition.x - player->getPositionX();
		float diffy = ballNextPosition.y - player->getPositionY();
		float distance1 = pow(diffx, 2) + pow(diffy, 2);
		float distance2 = pow(_ball->getPositionX() - playerNextPosition.x, 2) + pow(_ball->getPositionY() - playerNextPosition.y, 2);

		if (distance1 <= squared_radii || distance2 <= squared_radii)
		{
			float mag_ball = pow(ballVector.x, 2) + pow(ballVector.y, 2);
			float mag_player = pow(playerVector.x, 2) + pow(playerVector.y, 2);
			float force = sqrt(mag_ball + mag_player);
			float angle = atan2(diffy, diffx);
			ballVector.x = force * cos(angle);
			ballVector.y = (force * sin(angle));
			ballNextPosition.x = playerNextPosition.x + (player->radius() + _ball->radius() + force) * cos(angle);
			ballNextPosition.y = playerNextPosition.y + (player->radius() + _ball->radius() + force) * sin(angle);
			//SimpleAudioEngine::getInstance() -> playEffect("hit.wav");
			playHitSfx();
		}
	}

	//Check for collisions between ball and screen sides X axis
	if (ballNextPosition.x < _ball->radius()) {
		ballNextPosition.x = _ball->radius();
		ballVector.x *= -0.8f;
		//SimpleAudioEngine::getInstance()->playEffect("hit.wav");
		playHitSfx();
	}
	if (ballNextPosition.x > _screenSize.width - _ball->radius()) {
		ballNextPosition.x = _screenSize.width - _ball->radius();
		ballVector.x *= -0.8f;
		//SimpleAudioEngine::getInstance()->playEffect("hit.wav");
		playHitSfx();
	}

	//Check for collisions between ball and screen sides Y axis, also consider goal
	if (ballNextPosition.y > _screenSize.height - _ball->radius()) {
		if (_ball->getPosition().x < _screenSize.width * 0.5f - GOAL_WIDTH * 0.5f || _ball->getPosition().x >_screenSize.width * 0.5f + GOAL_WIDTH * 0.5f) {
			ballNextPosition.y = _screenSize.height - _ball->radius();
			ballVector.y *= -0.8f;
			//SimpleAudioEngine::getInstance()->playEffect("hit.wav");
			playHitSfx();
		}
	}
	if (ballNextPosition.y < _ball->radius()) {
		if (_ball->getPosition().x < _screenSize.width * 0.5f - GOAL_WIDTH * 0.5f || _ball->getPosition().x >
			_screenSize.width * 0.5f + GOAL_WIDTH * 0.5f) {
			ballNextPosition.y = _ball->radius();
			ballVector.y *= -0.8f;
			//SimpleAudioEngine::getInstance()->playEffect("hit.wav");
			playHitSfx();
		}
	}

	_ball->setVector(ballVector);
	_ball->setNextPosition(ballNextPosition);
	//Check for goals!
	if (ballNextPosition.y < -_ball->radius() * 2) {
		this->playerScore(2);
	}
	if (ballNextPosition.y > _screenSize.height + _ball->radius() * 2)
	{
		this->playerScore(1);
	}

	_player1->setPosition(_player1->getNextPosition());
	_player2->setPosition(_player2->getNextPosition());
	_ball->setPosition(_ball->getNextPosition());
}

Scene* GameLayer::scene()
{
	auto sc = Scene::create();
	auto gL = GameLayer::create();
	sc->addChild(gL);
	return sc;
}

void GameLayer::onTouchesBegan(const std::vector<Touch*>& touches, Event* event)
{
	for (auto touch : touches) {
		if (touch != nullptr) {
			auto tap = touch->getLocation();
			for (auto player : _players) {
				if (player->getBoundingBox().containsPoint(tap)) {
					player->setTouch(touch);
				}
			}
		}
	}
}

void GameLayer::onTouchesMoved(const std::vector<Touch*>& touches, Event* event) {
	
	for (auto touch : touches) {
		if (touch != nullptr) {
			auto tap = touch->getLocation();
				for (auto player : _players) {
					
					if (player->getTouch() != nullptr && player->getTouch() == touch) {
						Point nextPosition = tap;
						if (nextPosition.x < player->radius())
							nextPosition.x = player->radius();
						if (nextPosition.x > _screenSize.width - player->radius())
							nextPosition.x = _screenSize.width - player->radius();
						if (nextPosition.y < player->radius())
							nextPosition.y = player->radius();
						if (nextPosition.y > _screenSize.height - player->radius())
							nextPosition.y = _screenSize.height - player->radius();
						//keep player inside its court
						if (player->getPositionY() < _screenSize.height * 0.5f) {
							if (nextPosition.y > _screenSize.height * 0.5 -
								player->radius()) {
								nextPosition.y = _screenSize.height * 0.5 -
									player->radius();
							}
						}
						else {
							if (nextPosition.y < _screenSize.height * 0.5 +
								player->radius()) {
								nextPosition.y = _screenSize.height * 0.5 +
									player->radius();
							}
						}

						player->setNextPosition(nextPosition);
						player->setVector(Vec2(tap.x - player->getPositionX(), tap.y - player->getPositionY()));
					}
			}
		}
	}
}

void GameLayer::onTouchesEnded(const std::vector<Touch*>& touches, Event* event)
{
	for (auto touch : touches) {
		if (touch != nullptr) {
			auto tap = touch->getLocation();
			{
				for (auto player : _players) {
					if (player->getTouch() != nullptr && player->getTouch() == touch) {
						//if touch ending belongs to this player, clear it
						player->setTouch(nullptr);
						player->setVector(Vec2(0, 0));
					}
				}
			}
		}
	}
}

void GameLayer::playHitSfx() {
	_audioID = AudioEngine::play2d("sfx/sfx_puck.mp3", false, 0.85f);
}

void GameLayer::playScoreSfx() {
	_audioID = AudioEngine::play2d("sfx/sfx_score.mp3", false, 1);
}
