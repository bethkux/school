

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "Board.hpp"

std::string resourcesDir() {
    return "resources/";
}


////////////////////////////////////////////////////////////
// Constants, variables 
////////////////////////////////////////////////////////////
#pragma region Resources
// Resources
sf::RenderWindow window;
sf::SoundBuffer itemSoundBuffer;
sf::Sound itemSound;
sf::Texture snakeTexture;
sf::Sprite snakeSprite;
sf::Texture tileTexture;
sf::Sprite tileSprite;
sf::Font font;
sf::Text pauseMessage;

// Constatnts
const IntT dim = 16;
const IntT startingLength = 2;
const float gameWidth = 800;
const float gameHeight = gameWidth;

// Variables
sf::Vector2u tileSize;
Board board;
enum Direction { Up, Down, Left, Right };
Direction direction = Left, nextDirection = Left;
bool isPlaying = false, isAutoPlaying = false;
float timer = 0.0, delay = 0.1;
#pragma endregion


////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////

// Draw tileSprite with given scale, coords and color
static void draw(float scale, IntT coords, sf::Color color) {
    tileSprite.setPosition(scale * (coords % board.size()), scale *(coords / board.size()));
    tileSprite.setScale(scale/ tileSize.x, scale / tileSize.y);
    tileSprite.setColor(color);
    window.draw(tileSprite);
}

// Sets an appropriate string for when the game is over
static std::string endingString(size_t score) {
    return "\t\t\t\t   Score: " + std::to_string(score - startingLength) + "\n\n\t   Press S to start the game,\n\t    A to start the auto mode\n\t\t\t  or escape to exit.";
}


////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \return Application exit code
///
////////////////////////////////////////////////////////////
int main()
{
    sf::Clock clock;

    #pragma region Resources
    // Create the window of the application
    window.create(sf::VideoMode(static_cast<unsigned int>(gameWidth), static_cast<unsigned int>(gameHeight), 32), "Snake Game", sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    // Load the sounds used in the game
    if (!itemSoundBuffer.loadFromFile(resourcesDir() + "item.wav"))
        return EXIT_FAILURE;
    itemSound.setBuffer(itemSoundBuffer);

    // Create the snake image texture:
    if (!snakeTexture.loadFromFile(resourcesDir() + "snake.png"))
        return EXIT_FAILURE;
    snakeSprite.setTexture(snakeTexture);
    snakeSprite.setScale(gameWidth/1000, gameHeight/1000);
    snakeSprite.setPosition((gameWidth - 320 * (gameWidth / 1000)) / 2 + 5, gameHeight/6);

    // Create the snake body part texture:
    if (!tileTexture.loadFromFile(resourcesDir() + "tile.png"))
        return EXIT_FAILURE;
    tileSprite.setTexture(tileTexture);
    tileSize = tileTexture.getSize();

    // Load the text font
    if (!font.loadFromFile(resourcesDir() + "tuffy.ttf"))
        return EXIT_FAILURE;

    // Initialize the pause message
    pauseMessage.setFont(font);
    pauseMessage.setCharacterSize(40);
    pauseMessage.setPosition(120.f, gameHeight / 2);
    pauseMessage.setFillColor(sf::Color::White);
    pauseMessage.setString("\t  Welcome to Snake Game!\n\n    Press S to start the game or\n  press A to start the auto mode.");
    #pragma endregion

    // Application is running
    while (window.isOpen()) {
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {

            // Window closed or escape key pressed: exit
            if ((event.type == sf::Event::Closed) ||
                ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))) {
                window.close();
                break;
            }

            // Key pressed: play or auto-play
            if (event.type == sf::Event::KeyPressed && !isPlaying && !isAutoPlaying) {

                if (event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::A) {

                    board = Board(dim, startingLength);
                    clock.restart();
                    timer = -delay;

                    if (event.key.code == sf::Keyboard::S) {
                        isPlaying = true;
                        direction = Left, nextDirection = Left;
                    }
                    else if (event.key.code == sf::Keyboard::A) {
                        isAutoPlaying = true;
                    }
                }
            }

            // Window size changed, adjust view appropriately
            if (event.type == sf::Event::Resized) {
                sf::View view;
                view.setSize(gameWidth, gameHeight);
                view.setCenter(gameWidth / 2.0f, gameHeight / 2.0f);
                window.setView(view);
            }
        }

        // Playing normal mode
        if (isPlaying) {    

            #pragma region Controls
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && direction != Down) nextDirection = Up;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && direction != Up) nextDirection = Down;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && direction != Right) nextDirection = Left;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && direction != Left) nextDirection = Right;
            #pragma endregion

            // Snake is ready to be moved
            if (timer > delay) {
                timer = 0;
                direction = nextDirection;
                
                auto& snake = board.snake();
                IntT new_head = board.head(snake);

                // New position for head
                switch (direction) {
                case Up:
                    new_head -= board.size();
                    break;
                case Down:
                    new_head += board.size();
                    break;
                case Left:
                    new_head -= 1;
                    break;
                case Right:
                    new_head += 1;
                    break;
                }

                // Game over - LOSE
                if (!board.is_inside(new_head) || (board.contains(snake, new_head) && board.tail(snake) != new_head)) {
                    isPlaying = false;
                    pauseMessage.setString("\t\t\t\t  You Lost!\n" + endingString(board.snake_length()));
                }
                // Item eaten
                else if (new_head == board.item()) {
                    board.set_snake(board.shift(new_head, snake, true));

                    // Game over - WIN
                    if ((IntT)snake.size() == (board.size() * board.size() - 4 * (board.size() - 1))) {
                        isPlaying = false;
                        pauseMessage.setString("\t\t\t\t  You Won!\n" + endingString(board.snake_length()));
                    }
                    // Find new position for item
                    else {
                        board.set_item(board.generate_item());
                        itemSound.play();
                    }
                }
                // Move snake
                else
                    board.set_snake(board.shift(new_head, snake, false));
            }
        }
        // Playing auto mode
        else if (isAutoPlaying) {

            // Snake is ready to be moved
            if (timer > delay) {
                timer = 0;

                // Find new path to follow
                if (board.isPathEmpty()) {
                    // Game over
                    if (board.gameOver()) {
                        isAutoPlaying = false;
                        pauseMessage.setString("\t\t\t\t Game over!\n" + endingString(board.snake_length()));
                    }
                    // Continue running
                    else
                        board.autoPilotStep();
                }

                // If there is still path left, follow it
                if (!board.isPathEmpty()) {
                    if (board.shift_snake()) 
                        itemSound.play();
                }
            }
        }

        #pragma region Drawing Board
        // Clear the window
        window.clear(sf::Color(50, 50, 50));

        if (isPlaying || isAutoPlaying) {
            float scale = gameWidth / board.size();

            // Wall
            for (IntT tile = 0; tile < board.size() * board.size(); ++tile) {
                if (!board.is_inside(tile)) 
                    draw(scale, tile, sf::Color::Black);
            }

            // Item
            draw(scale, board.item(), sf::Color::Red);

            // Snake
            auto& snake = board.snake();
            int gradient = 255 / (snake.size() - 1);
            for (IntT i = 0; i < (IntT)snake.size(); ++i) {
                draw(scale, snake[i], sf::Color(gradient * i, 250, gradient * i));
            }
        }
        else {
            // Draw the pause message
            window.draw(pauseMessage);
            window.draw(snakeSprite);
        }

        // Display things on screen
        window.display();
        #pragma endregion
    }

    return EXIT_SUCCESS;
}
