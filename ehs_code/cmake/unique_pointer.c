/* check std::unique_pts and std::move */
#include <memory>

class foo {
	public: 
		int bar; 
};

void mv(std::unique_ptr<foo>p) {
	 int v = p->bar;
}

int main()
{
	std::unique_ptr<foo>p(new foo());
	mv(std::move(p));
	return 0;
}