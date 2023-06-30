#include <iostream>
#include <string>

#include "Shape.h"

struct Square {
  std::string name;
  int sideLength;

  void draw() const { std::cout << "Square " << name << " " << sideLength << " draw\n"; }
  void setName(std::string value) { name = std::move(value); }

  friend void draw(const Square& square) {
    std::cout << "Square " << square.name << " " << square.sideLength << " draw free\n";
  }
  friend void setName(Square& square, std::string value) { square.name = std::move(value); }

  ~Square() { std::cout << "Square " << name << " " << sideLength << " destroyed\n"; }
};

int main() {
  Square sq{"MySquare", 3};

  sq.draw();
  draw(sq);

  Shape shape{sq};
  ShapeRef shapeRef{sq};
  ShapeConstRef shapeConstRef{sq};

  shape.draw();
  shapeRef.draw();
  shapeConstRef.draw();

  draw(shape);
  draw(shapeRef);
  draw(shapeConstRef);

  const Shape cShape{sq};
  const ShapeRef cShapeRef{sq};
  const ShapeConstRef cShapeConstRef{sq};

  cShape.draw();
  cShapeRef.draw();
  cShapeConstRef.draw();

  draw(cShape);
  draw(cShapeRef);
  draw(cShapeConstRef);

  auto ui{cShape};
  auto bla{cShapeRef};
  auto blubb{cShapeConstRef};

  draw(ui);
  draw(bla);
  draw(blubb);

  auto uiM{std::move(ui)};
  auto blaM{std::move(bla)};
  auto blubbM{std::move(blubb)};

  draw(uiM);
  draw(blaM);
  draw(blubbM);

  Shape assign{Square{"AnotherSquare", 4}};

  draw(assign);

  auto tmp{assign};

  assign = uiM;

  draw(uiM);
  draw(assign);

  assign = std::move(tmp);

  draw(assign);

  Square testSetNameSquare{"TestSetNameSquare", 23};
  Shape testSetNameSquareShape{testSetNameSquare};
  ShapeRef testSetNameSquareShapeRef{testSetNameSquare};
  ShapeConstRef testSetNameSquareShapeConstRef{testSetNameSquare};

  std::cout << "---\n";

  draw(testSetNameSquare);
  draw(testSetNameSquareShape);
  draw(testSetNameSquareShapeRef);
  draw(testSetNameSquareShapeConstRef);

  std::cout << "---\n";

  setName(testSetNameSquare, "Wurst");

  draw(testSetNameSquare);
  draw(testSetNameSquareShape);
  draw(testSetNameSquareShapeRef);
  draw(testSetNameSquareShapeConstRef);

  std::cout << "---\n";

  setName(testSetNameSquareShapeRef, "Käse");

  draw(testSetNameSquare);
  draw(testSetNameSquareShape);
  draw(testSetNameSquareShapeRef);
  draw(testSetNameSquareShapeConstRef);

  std::cout << "---\n";

  setName(testSetNameSquareShape, "Schinkenspicker");

  draw(testSetNameSquare);
  draw(testSetNameSquareShape);
  draw(testSetNameSquareShapeRef);
  draw(testSetNameSquareShapeConstRef);

  std::cout << "---\n";

  testSetNameSquareShape.setName("Käsekrüstchen");

  draw(testSetNameSquare);
  draw(testSetNameSquareShape);
  draw(testSetNameSquareShapeRef);
  draw(testSetNameSquareShapeConstRef);

  std::cout << "---\n";

  // testSetNameSquareShapeConstRef.setName("Hallo");

  const ShapeRef constTestSetNameSquareShapeRef{testSetNameSquare};

  constTestSetNameSquareShapeRef.setName("Hallo");

  const Shape constTestSetNameSquare{testSetNameSquare};

  // constTestSetNameSquare.setName("Hallo");

  return 0;
}
