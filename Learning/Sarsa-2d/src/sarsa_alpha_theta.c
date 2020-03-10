#include "../../Dynamics/dynamics_2d_spherical.h"
#include "sarsa_alpha_theta.h"

#define decision_time 1000

//#define theta0 PI/4.
#define vtheta0 0.

double reward_dt = h*decision_time;

int main(int argc, char *argv[]){

    FILE *out ,*rew, *Q_mat, *policy, *Q_mat_counter;
    out = fopen("out.txt", "w");
    rew = fopen("rewards.txt", "w");
    Q_mat = fopen("Q_mat.txt", "w");
    Q_mat_counter = fopen("Q_count.txt", "w");
    policy = fopen("policy.txt", "w");

    fprintf(out, "t         x_kite          z_kite         x_blocco          z_blocco          wind_x       wind_y       v_blocco_x\n");
    //fprintf(Q_mat, "t       Q1      Q2      Q3      Q4      Q5      Q6      Q7      Q8      Q9      Q10     Q11     Q12      Q13     Q14\n");
    fprintf(policy, "step        alpha       action      reward        Q[s+0]      Q[s+1]      Q[s+2]\n");
    
    // vettori moto kite dall'origine fissa (x, z)
    double *rk = (double*) malloc(2 * sizeof(double)); 
    double *vk = (double*) malloc(2 * sizeof(double)); 
    double *ak = (double*) malloc(2 * sizeof(double)); 

    // vettori moto blocco dall'origine fissa (x, z)
    double *x_blocco = (double*) malloc(2 * sizeof(double)); 
    double *v_blocco = (double*) malloc(2 * sizeof(double));
    double *a_blocco = (double*) malloc(2 * sizeof(double));  
    
    // theta, dtheta, ddtheta
    double *theta = (double*) malloc(3 * sizeof(double));  
    double T = 0;

    double * Q = (double*) malloc(n_alphas * n_actions * n_theta *  sizeof(double));
    int * Q_counter = (int*) malloc(n_alphas * n_actions * n_theta *  sizeof(int));

    double reward = 0;
    double tot_reward = 0;
    double penalty = -1000;
    double premio = 500;

    double W[2];
    double lift=0, drag=0;

    // SARSA STATE
    int s_alpha, s_alpha1;
    int s_theta, s_theta1;

    // SARSA ACTION
    int a_alpha = 0, a_alpha1 = 0;           

    int episode = 0;

    double max_pos[2] = {0, R};

    streamfunction(max_pos, &W[0], &W[1]);
    printf("W[0]=%f\n", W[0]);
    printf("W[1]=%f\n", W[1]);
    double W_max = sqrt(W[0]*W[0] + W[1]*W[1]);

    double reward_max = W_max*max_steps*h;

    double theta0;
    
    printf("W max %f\n\n", W_max);
    printf("reward max %f\n\n", reward_max);

    initialize_Q(Q, 4000);
    initialize_Q_counter(Q_counter, 0);

    printf("Total number of episodes: %d\n", learning_episodes);

    // ======================= STARTING EPISODES ==========================

    while (episode < learning_episodes){

        if (episode == (int)(learning_episodes/4)){
            Alpha = Alpha*0.1;
            epsilon = 0.85;
            printf("Decreasing learning rate: %f\n", Alpha);
            printf("Decreasing exploration rate (increasing epsilon): %f\n", epsilon);
        }
        if (episode == (int)(learning_episodes/2)){
            Alpha = Alpha*0.1;
            epsilon = 0.90;
            printf("Decreasing learning rate: %f\n", Alpha);
            printf("Decreasing exploration rate (increasing epsilon): %f\n", epsilon);
        }
        if (episode == (int)(learning_episodes*3/4)){
            Alpha = Alpha*0.1;
            epsilon = 0.96;
            printf("Decreasing learning rate: %f\n", Alpha);
            printf("Decreasing exploration rate (increasing epsilon): %f\n", epsilon);
        }
        if (episode == learning_episodes-1){
            epsilon = 0.99;
        }
    
        // ======================= EPISODE INITIALIZATION ==========================

        printf("Episode: %d\nepsilon: %f\n", episode, epsilon);
        printf("Learning rate: %f\n", Alpha);

        theta0 = PI/4; 
        s_alpha = 10;
	
        variables_initialization(x_blocco, v_blocco, a_blocco, theta, rk, vk, ak, theta0, vtheta0);
        
        s_theta = find_theta_state(theta[0]);
        a_alpha = select_alpha_action(epsilon, Q, s_alpha, s_theta, episode);

        streamfunction(rk, &W[0], &W[1]);

        printf("initial state=%d\n", s_alpha);
        printf("theta0 = %f, vel_theta0 = %f\n", theta[0], theta[1]);
        printf("W[0]=%f\n", W[0]);
        printf("W[1]=%f\n", W[1]);

        integration_trajectory(rk, vk, ak, x_blocco, v_blocco, a_blocco, theta, &T, \
                               s_alpha, W, &lift, &drag);

        int it = 0;

        reward = 0;
        tot_reward = 0.; 
        
        while (rk[1] > 0){

            if (it > max_steps){
                printf("MAX STEPS, %d, exiting\n", max_steps);
                printf("return=%f, space percurred=%f\n\n", tot_reward, x_blocco[0]);
                fprintf(rew, "%d    %f\n", episode, tot_reward);

                // fill Q matrix file
                fill_Q_mat(Q_mat, Q);

                // fill Q counter file
                if (episode == learning_episodes - 1){
                    fill_Q_counter_file(Q_mat_counter, Q_counter, episode);
                }

                break;
            }

            streamfunction(rk, &W[0], &W[1]);

            integration_trajectory(rk, vk, ak, x_blocco, v_blocco, a_blocco, theta, &T, \
                                   s_alpha, W, &lift, &drag);

            if (it%decision_time == 0){
                reward = fabs(v_blocco[0])*reward_dt;
                tot_reward += reward;
            }
            
            // SOME CHECKS
            /*if (check(s_alpha, a_alpha, s_theta) == 1) {
                printf("CHECK: s_alpha %d\n", s_alpha);
                printf("Matrix index error!!\n");
                episode = learning_episodes;
                break;
            }

            if (a_alpha > 2 || a_alpha < 0){
                printf("a_alpha:%d\n", a_alpha1);
                printf("ERROR IN UPDATE STATE");
                episode = learning_episodes;
                break;
            }*/

            // save data of last episode for plot
            if ( (episode == learning_episodes - 1) && (it%decision_time == 0) ){
                fprintf(out, "%d       %f       %f      %f      %f      %f      %f     %f\n", it, \
                    rk[0], rk[1], x_blocco[0], x_blocco[1], W[0], W[1], v_blocco[0]);
                
                fprintf(policy, "%d      %f     %d     %f     %f     %f     %f\n", \
                it, alphas[s_alpha], a_alpha, reward, Q[s_alpha*n_actions*n_theta + 0*n_theta + s_theta], \
                Q[s_alpha*n_actions*n_theta + 1*n_theta + s_theta], Q[s_alpha*n_actions*n_theta + 2*n_theta + s_theta]);
           
            }

            if (rk[1] <= 0.) {
                fprintf(rew, "%d    %f\n", episode, tot_reward);

                // fill Q matrix file
                fill_Q_mat(Q_mat, Q);

                // fill Q counter file
                if (episode == learning_episodes - 1){
                    fill_Q_counter_file(Q_mat_counter, Q_counter, episode);
                }

                printf("Kite fallen: z<0, steps=%d, break\n", it);
                printf("return=%f, space percurred=%f\n\n", tot_reward, x_blocco[0]);

                Q[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta] +=
                    Alpha*(reward + penalty - Q[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta]);
                
                Q_counter[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta] += 1;

                tot_reward = 0;
                break;
            }

            if (it%decision_time == 0){

                // FIND STATE S1 USING ACTION A

                s_alpha1 = update_state(s_alpha, a_alpha);
                s_theta1 = find_theta_state(theta[0]);

                // SEARCH FOR NEXT ACTION A1

                a_alpha1 = select_alpha_action(epsilon, Q, s_alpha1, s_theta1, episode);

                // UPDATE Q MATRIX
                if (it == max_steps){
                    Q[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta] += \
                        Alpha*(reward + premio - Q[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta]);
                    
                    Q_counter[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta] += 1;
                }
                else {
                    Q[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta] += \
                        Alpha*(reward + Gamma*Q[s_alpha1*n_actions*n_theta + a_alpha1*n_theta + s_theta1] \
                        - Q[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta]);
                    
                    Q_counter[s_alpha*n_actions*n_theta + a_alpha*n_theta + s_theta] += 1;
                }

                // MOVE ON: S = S1, A = A1

                s_alpha = s_alpha1;
                s_theta = s_theta1;

                a_alpha = a_alpha1;
                
            }

            it += 1;
        }

        episode += 1;

    }

    print_mat(Q);

    //printf("save matrix\n");
    save_matrix(Q, "Q_matrix.dat");
    //printf("load matrix\n");
    //load_matrix(Q2, "Q_matrix.dat");
    //print_mat(Q2);

    free(rk);
    free(vk);
    free(ak);
    free(Q);
    free(Q_counter);
    free(x_blocco);
    free(v_blocco);
    free(a_blocco);
    free(theta);

    fclose(out);
    fclose(rew);
    fclose(Q_mat);
    fclose(Q_mat_counter);
    fclose(policy);

    return 0;

    }
