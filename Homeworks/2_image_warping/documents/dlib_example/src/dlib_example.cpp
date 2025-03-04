#include <dlib/dnn.h>
#include <dlib/rand.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace dlib;

// define the network
using net_type = loss_mean_squared<    // loss mean squared
                fc<1,                  // output layer: 1 dim
                relu<fc<10,            // hidden layer 2: 50 dim + ReLU activation
                relu<fc<10,            // hidden layer 1: 50 dim + ReLU activation
                input<matrix<float>>   // input layer: 1 dim
                >>>>>>; 

int main()
{
    // Generate training data by y = sin(x)
    dlib::rand rnd;
    std::vector<matrix<float>> inputs;
    std::vector<float> targets;
    const double pi = 3.1415926535;
    for (double x = -2*pi; x < 2*pi; x += 0.01) {
        matrix<float> in(1,1), out(1,1);
        in = x;                          
        out = sin(x) + 0.1*rnd.get_random_gaussian(); 
        inputs.push_back(in);
        targets.push_back(out);
    }

    // the network
    net_type net;

    // hyper parameters for training
    dnn_trainer<net_type> trainer(net, sgd(0.01, 0.5)); // sgd optimizer with learning rate 0.01
    trainer.set_learning_rate(0.01);               
    trainer.set_min_learning_rate(1e-5);           
    trainer.set_mini_batch_size(32);               
    trainer.set_max_num_epochs(1000);               
    trainer.be_verbose();                          // print training progress
    // train the network
    trainer.train(inputs, targets);

    // test the network
    matrix<float> test_input(1,1);
    test_input = 0.5f;
    float prediction = net(test_input);
    std::cout << "prediction: " << prediction 
         << "ground truth" << sin(0.5f) 
         << std::endl;

    return 0;
}