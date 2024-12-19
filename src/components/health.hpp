
struct Health {
  int value;
  Health(int value = 0) : value(value) {}
  static const Health Invalid;
};

const Health Health::Invalid = Health(-1);
