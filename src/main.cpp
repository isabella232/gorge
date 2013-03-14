#include <cstdio>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <forward_list>

#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "helper.hpp"
#include "object.hpp"
#include "walls.hpp"


using namespace std;





class Star : public Object {
public:
	Star() {
		init("media/star.png");
		speed = randFloat(0.3, 1);
		float c = (speed - 0.3) * 100;
		float d = c * randFloat(1,2);
		setColor(sf::Color(c, d, d));
		reSet();
		setPosition(randFloat(-10, 810), randFloat(-10, 610));
	}
	virtual bool update() {
		move({0, speed});
		if (getPosition().y > 632) reSet();
		return true;
	}
	virtual void draw(sf::RenderWindow& win) {
		win.draw(*this, sf::BlendAdd);
	}
	float getSpeed() const { return speed; };
private:
	void reSet() {
		setPosition(randFloat(-32, 832), -32);
		setFrame(randInt(0, 20) == 0);
	}
	float speed;
};


Walls walls;


class Laser : public Object {
public:
	Laser(Vec2 pos) {
		init("media/laser.png");
		setPosition(pos);
	}
	virtual bool update() {
		move(vel);
		updateCollisionPoly();

		Vec2 normal, where;
		float distance = walls.checkCollision(poly, normal, where);
		if (distance > 0) return false;

		if (getPosition().y < -16) return false;
		return true;
	}
private:
	Vec2 vel = {0, -16};

	virtual const Poly& getCollisionModel() {
		static const Poly model = {
			Vec2(0.5, 2.5),
			Vec2(0.5, -2.5),
			Vec2(-0.5, -2.5),
			Vec2(-0.5, 2.5),
		};
		return model;
	}
};


forward_list<unique_ptr<Laser>> lasers;




class Player : public Object {
public:
	virtual void init() {
		Object::init("media/ship.png");
		setPosition(400, 500);
	}

	virtual bool update() {

		float joy_x = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
		float joy_y = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
		Vec2 mov;
		mov.x = (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) or joy_x > 50) -
				(sf::Keyboard::isKeyPressed(sf::Keyboard::Left) or joy_x < -50);
		mov.y = (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) or joy_y > 50) -
				(sf::Keyboard::isKeyPressed(sf::Keyboard::Up) or joy_y < -50);
		Vec2 pos = getPosition();
		pos += mov * 3.0f;
		pos.x = min(pos.x, 784.0f);
		pos.x = max(pos.x, 16.0f);
		pos.y = min(pos.y, 584.0f);
		pos.y = max(pos.y, 16.0f);
		setPosition(pos);
		updateCollisionPoly();
		Vec2 normal, where;
		float distance = walls.checkCollision(poly, normal, where);
		if (distance > 0) {
			// TODO: explode here
			move(normal * -distance);
			pos = getPosition();
		}


		bool shoot =	sf::Joystick::isButtonPressed(0, 0) |
						sf::Keyboard::isKeyPressed(sf::Keyboard::X);

		if (shoot) {
			if (shootDelay == 0) {
				lasers.push_front(
					unique_ptr<Laser>(new Laser(pos + Vec2(0, -10))));
				shootDelay = 10;
			}
			else shootDelay--;
		}
		else shootDelay = 0;


		setFrame(tick++ / 4 % 2);

		return true;
	}


private:
	virtual const Poly& getCollisionModel() {
		static const Poly model = {
			Vec2(4, 4),
			Vec2(4, 1),
			Vec2(1, -4),
			Vec2(-1, -4),
			Vec2(-4, 1),
			Vec2(-4, 4),
		};
		return model;
	}

	size_t shootDelay;
	size_t tick = 0;
};


class Enemy : public Object {
public:
private:
};


class Cannon : public Enemy {
public:
	Cannon(Vec2 pos, float ang) {
		init("media/cannon.png");
		setPosition(pos);
		angle = ang;

	}
	virtual bool update() {
		move(0, walls.getSpeed());
		setRotation(angle);

		updateCollisionPoly();
		return true;
	}
	float q = 0;
	virtual void draw(sf::RenderWindow& win) {
		q += 0.03;
		setRotation(angle + sin(q) * 90);
		setFrame(1);
		win.draw(*this);

		setRotation(angle);
		setFrame(0);
		win.draw(*this);

//		drawPoly(win, poly);

	}

private:
	float angle;


	virtual const Poly& getCollisionModel() {
		static const Poly model = {
			Vec2(4, 4),
			Vec2(4, -5),
			Vec2(-4, -5),
			Vec2(-4, 4),
		};
		return model;
	}
};






forward_list<unique_ptr<Enemy>> enemies;
Player player;
vector<Star> stars;


void update() {
	for (auto& star: stars) star.update();

	static int i = 0;
	if (!(i++ % 200)) {

		struct Location { Vec2 pos; float ang; };
		vector<Location> locs;

		for (int x = 1; x < walls.width - 1; x++) {
			Vec2 pos = Vec2((x - 0.5) * 32, (19.5 - 21) * 32 + walls.offset);
			if (walls.getTile(21, x) == 0) {
				if (walls.getTile(22, x) == 1) locs.push_back({ pos, 180 });
				if (walls.getTile(21, x - 1) == 1) locs.push_back({ pos, 90 });
				if (walls.getTile(21, x + 1) == 1) locs.push_back({ pos, 270 });
				if (walls.getTile(20, x) == 1) locs.push_back({ pos, 0 });
			}
		}

		if (!locs.empty()) {
			Location& loc = locs[randInt(0, locs.size() - 1)];
			enemies.push_front(
				unique_ptr<Enemy>(new Cannon(loc.pos, loc.ang)));
		}



	}

	walls.update();
	updateList(lasers);
	player.update();

	updateList(enemies);
}


void draw(sf::RenderWindow& win) {
	sf::View view = win.getDefaultView();
	view.zoom(1.2);
	win.setView(view);
	static const Poly frame {{0,0}, {800, 0}, {800, 600}, {0, 600}};



	for (auto& star : stars) star.draw(win);

	for (auto& laser : lasers) laser->draw(win);
	player.draw(win);
	for (auto& enemy : enemies) enemy->draw(win);

	walls.draw(win);





	drawPoly(win, frame);
}


int main(int argc, char** argv) {
	srand((unsigned)time(nullptr));

	sf::RenderWindow window(sf::VideoMode(800, 600), "sfml",
							sf::Style::Titlebar || sf::Style::Close);
	window.setFramerateLimit(60);
	window.setMouseCursorVisible(false);
	walls.init();
	player.init();

	stars.resize(100);
	sort(stars.begin(), stars.end(), [](const Star& a, const Star& b) {
		return a.getSpeed() < b.getSpeed();
	});


	while (window.isOpen()) {
		sf::Event e;
		while (window.pollEvent(e)) {
			switch (e.type) {
			case sf::Event::Closed:
				window.close();
				break;

			case sf::Event::KeyPressed:
				if (e.key.code == sf::Keyboard::Escape) window.close();
				break;

			default:
				break;
			}

		}
		update();
		window.clear();
		draw(window);
		window.display();
	}

	return 0;
}

