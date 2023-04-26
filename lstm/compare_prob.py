import numpy as np

cca_set = [1,6]

for test_data in cca_set:
    correct_prob = list()
    wrong_prob = list()
    for train_data in cca_set:
        prob_list = np.fromfile('prob/prob_train'+str(train_data)+'_test'+str(test_data)+'.dat', dtype=float)
        if train_data == test_data:
            correct_prob = prob_list
        else:
            wrong_prob = prob_list

    assert len(correct_prob) == len(wrong_prob)
    num_correct_label = 0
    for i in range(len(correct_prob)):
        if correct_prob[i] >= wrong_prob[i]:
            num_correct_label += 1

    print(f'test data from cca {test_data}, training data from cca {train_data}: {len(correct_prob)} slices in total, {num_correct_label} correct')
