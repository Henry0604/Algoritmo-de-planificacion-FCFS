#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Simulador de Procesos");
    setFixedSize(300,150);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::generateProcesses(){
    Proceso p;
    int estimatedTime,num1,num2,signOperator,i;
    //Generating random numbers
    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution_time(6,16);
    std::uniform_int_distribution<int> distribution_numbers(-100,100);
    std::uniform_int_distribution<int> distribution_sign(0,5);
    auto randomNumber = bind(distribution_numbers, generator);
    auto randomSign = bind(distribution_sign,generator);
    auto randomEstimatedTime = bind(distribution_time, generator);

    i = 0;
    while (i < totalProcess){
        i++;
        p.setId(i);
        num1 = randomNumber();
        num2 = randomNumber();
        signOperator = randomSign();
        createOperation(p,num1,num2,signOperator);
        estimatedTime = randomEstimatedTime();
        p.setTiempoEstimado(estimatedTime);
        p.setTiempoMaximoEstimado(estimatedTime);
        newProcesses.push_back(p);
        newProcessesCount++;
    }
}

void MainWindow::createOperation(Proceso &p,int num1,int num2,int signOperator)
{
    int result;
    std::string operation = "";
    char sign;
    //Generating random numbers with the system time
    std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution_numbers(-100,100);
    std::uniform_int_distribution<int> distribution_sign(0,5);
    auto randomNumber = bind(distribution_sign,generator);

    switch (signOperator){
    case ADD:
        result = num1 + num2;
        sign = '+';
        break;
    case SUB:
        result = num1 - num2;
        sign = '-';
        break;
    case MULT:
        result = num1 * num2;
        sign = '*';
        break;
    case DIV:
        while (num2 == 0){
            num2 = randomNumber();
        }
        result = num1 / num2;
        sign = '/';
        break;
    case MOD:
        while (num2 == 0){
            num2 = randomNumber();
        }
        result = num1 % num2;
        sign = '%';
        break;
    default:
        sign = '+';
        result = num1 + num2;
        break;
    }
    operation = std::to_string(num1) + sign + std::to_string(num2);
    p.setOperacion(operation);
    p.setResultadoOperacion(result);
}

void MainWindow::loadProcessesMemory()
{
    //The maximum of processes in memory is 5
    for (int i = 0; i < MAX_PROCESSES_IN_MEMORY and i < totalProcess; i++){
        Proceso p;
        p = newProcesses[0];
        p.setTiempoLlegada(globalCounter);
        newProcesses.erase(newProcesses.begin());
        readyProcesses.push(p);
    }
    totalProcess > MAX_PROCESSES_IN_MEMORY ?
                newProcessesCount = newProcessesCount - MAX_PROCESSES_IN_MEMORY:
                newProcessesCount = 0;
}

void MainWindow::startProcesses()
{
    ui->newProcessesLCD->display(newProcessesCount);
    loadProcessesMemory();
    while (readyProcesses.size()){
        showReadyProcesses();
        runProcess();
        if (readyProcesses.size() == 0 and blockProcesses.size() > 0){
            Proceso nullP(0,"NULL",0,false);
            readyProcesses.push(nullP);
        }
    }
    //Notifying that processes have finished
    QMessageBox msg;
    msg.setWindowTitle("Atencion!");
    msg.setText("Han terminado de ejecutarse todos los procesos.");
    msg.exec();

    //In this point all the processes have finished
    calculateProcessesTimes();
    ui->stackedWidget->setCurrentIndex(SHOW_TIMES_PROCESES);
    showProcessesTimes();
}

void MainWindow::showReadyProcesses()
{
    int rows = 0;
    int i = 1;
    Proceso p;

    ui->readyProcessesTB->setRowCount(0);
    p = readyProcesses.front();
    readyProcesses.pop();
    readyProcesses.push(p);
    while (i < (int)readyProcesses.size()){
        p = readyProcesses.front();
        readyProcesses.pop();
        readyProcesses.push(p);
        ui->readyProcessesTB->insertRow(ui->readyProcessesTB->rowCount());
        ui->readyProcessesTB->setItem(rows,0,new QTableWidgetItem(QString::number(p.getId())));
        ui->readyProcessesTB->setItem(rows,1,new QTableWidgetItem(QString::number(p.getTiempoEstimado())));
        ui->readyProcessesTB->setItem(rows,2,new QTableWidgetItem(QString::number(p.getTiempoTranscurrido())));
        rows++;
        i++;
    }
}

void MainWindow::runProcess()
{
    Proceso& p = readyProcesses.front();

    //Process flags
    p.setFinalizacion(SUCCESFUL_FINISH);
    if (!p.getEjecutado()){
        p.setEjecutado(true);
        p.setTiempoRespuesta(globalCounter - p.getTiempoLlegada());
    }

    tT = p.getTiempoTranscurrido();
    tR = p.getTiempoEstimado() - tT;

    //Inicial states of the program in each process
    interrupted = error = false;

    //Printing current process running
    ui->processRuningTB->setItem(0,0,new QTableWidgetItem(QString::number(p.getId())));
    ui->processRuningTB->setItem(1,0,new QTableWidgetItem(QString(p.getOperacion().c_str())));
    ui->processRuningTB->setItem(2,0,new QTableWidgetItem(QString::number(p.getTiempoEstimado())));
    ui->processRuningTB->setItem(3,0,new QTableWidgetItem(QString::number(p.getTiempoTranscurrido())));
    ui->processRuningTB->setItem(4,0,new QTableWidgetItem(QString::number(tR)));

    if (p.getId() != NULL_PROCESS){
        //Increment de tT and decrement the tR
        while (tT < p.getTiempoEstimado() and tT != ACTION_CODE){
            tR--;
            tT++;
            p.setTiempoTranscurrido(tT);
            ui->processRuningTB->setItem(3,0,new QTableWidgetItem(QString::number(p.getTiempoTranscurrido())));
            ui->processRuningTB->setItem(4,0,new QTableWidgetItem(QString::number(tR)));

            //Blocked Process
            if (!blockProcesses.empty()){
                incrementBlockedTimes();
            }
            showBlockedProcesses();
            globalCounter++;
            ui->globalCountLCD->display(globalCounter);
            ui->newProcessesLCD->display(newProcessesCount);
            delay();
        }
        //Checking which kind of action is next
        if (interrupted){
            interruptProcesss(p);
        }
        else if (error){
            showFinishedProcesses(ERROR_FINISH,p);
        }
        else{
            showFinishedProcesses(SUCCESFUL_FINISH,p);
        }
    }
    else{
        //Comparing if there is another process further than the null process
        while (readyProcesses.size() < 2) {
            incrementBlockedTimes();
            globalCounter++;
            showBlockedProcesses();
            ui->globalCountLCD->display(globalCounter);
            ui->newProcessesLCD->display(newProcessesCount);
            delay();
        }
    }
    readyProcesses.pop();
    ui->stackedWidget->repaint();
    ui->processRuningTB->clearContents();
}

void MainWindow::showFinishedProcesses(bool finishedType,Proceso& p)
{
    if (p.getId() != NULL_PROCESS){
        //Setting finished time to a each process that finish
        p.setTiempoFinalizacion(globalCounter);
        p.setFinalizacion(finishedType);
        finishedProcesses.push_back(p);
        //Printing the table
        int rowsFinished(ui->finishedTB->rowCount());
        ui->finishedTB->insertRow(rowsFinished);
        ui->finishedTB->setItem(rowsFinished,0,new QTableWidgetItem(QString::number(p.getId())));
        ui->finishedTB->setItem(rowsFinished,1,new QTableWidgetItem(QString(p.getOperacion().c_str())));

        if (finishedType == ERROR_FINISH){
            ui->finishedTB->setItem(rowsFinished,2,new QTableWidgetItem(QString("ERROR")));
        }
        else{
            ui->finishedTB->setItem(rowsFinished,2,new QTableWidgetItem(QString::number(p.getResultadoOperacion())));
        }
        //Adding a new process
        if (newProcesses.size()){
            Proceso p;
            p = newProcesses[0];
            newProcesses.erase(newProcesses.begin());
            //Setting arrive time for each new process in ready processes
            p.setTiempoLlegada(globalCounter);
            readyProcesses.push(p);
            newProcessesCount--;
        }
    }
}

void MainWindow::interruptProcesss(Proceso& p)
{
    //Pasing the current process into the block queue
    blockProcesses.push(p);
    showBlockedProcesses();
}

void MainWindow::showBlockedProcesses()
{
    Proceso p;
    int i = 0,row = 0;

    //Showing the current values of the blocked processes
    ui->blockTB->setRowCount(0);
    while (i < (int)blockProcesses.size()){
        p = blockProcesses.front();
        blockProcesses.pop();
        blockProcesses.push(p);
        ui->blockTB->insertRow(ui->blockTB->rowCount());
        ui->blockTB->setItem(row,0,new QTableWidgetItem(QString::number(p.getId())));
        ui->blockTB->setItem(row,1,new QTableWidgetItem(QString::number(p.getTiempoBloqueado())));
        i++;
        row++;
    }
}

void MainWindow::incrementBlockedTimes()
{
    int bt,i;

    //Increment the blocked time of each process
    for (i = 0; i < (int)blockProcesses.size();){
        Proceso bp = blockProcesses.front();
        bt = bp.getTiempoBloqueado() + 1;
        blockProcesses.pop();
        bp.setTiempoBloqueado(bt);
        if (bp.getTiempoBloqueado() > BLOCKED_TIME){
            bp.setTiempoBloqueado(0);
            readyProcesses.push(bp);
            showReadyProcesses();
        }
        else{
            blockProcesses.push(bp);
            i++;
        }
    }
}

void MainWindow::calculateProcessesTimes()
{
    for (int i = 0; i < (int)finishedProcesses.size(); i++){
        Proceso& p = finishedProcesses[i];
        p.setTiempoRetorno(p.getTiempoFinalizacion()-p.getTiempoLlegada());
        p.setTiempoServicio(p.getTiempoTranscurrido());
        p.setTiempoEspera(p.getTiempoRetorno()-p.getTiempoServicio());
    }
}

void MainWindow::showProcessesTimes()
{
    int row;
    Proceso p;

    ui->timesProcessesTB->setRowCount(0);
    for (int i = 0; i < (int)finishedProcesses.size(); i++){
        p = finishedProcesses[i];
        row = ui->timesProcessesTB->rowCount();
        ui->timesProcessesTB->insertRow(row);
        ui->timesProcessesTB->setItem(row,0,new QTableWidgetItem(QString::number(p.getId())));
        ui->timesProcessesTB->setItem(row,1,new QTableWidgetItem(QString(p.getOperacion().c_str())));
        if (p.getFinalizacion()){
            ui->timesProcessesTB->setItem(row,2,new QTableWidgetItem(QString::number(p.getResultadoOperacion())));
        }
        else{
            ui->timesProcessesTB->setItem(row,2,new QTableWidgetItem(QString("ERROR")));
        }
        ui->timesProcessesTB->setItem(row,3,new QTableWidgetItem(QString::number(p.getTiempoMaximoEstimado())));
        ui->timesProcessesTB->setItem(row,4,new QTableWidgetItem(QString::number(p.getTiempoLlegada())));
        ui->timesProcessesTB->setItem(row,5,new QTableWidgetItem(QString::number(p.getTiempoFinalizacion())));
        ui->timesProcessesTB->setItem(row,6,new QTableWidgetItem(QString::number(p.getTiempoRetorno())));
        ui->timesProcessesTB->setItem(row,7,new QTableWidgetItem(QString::number(p.getTiempoRespuesta())));
        ui->timesProcessesTB->setItem(row,8,new QTableWidgetItem(QString::number(p.getTiempoEspera())));
        ui->timesProcessesTB->setItem(row,9,new QTableWidgetItem(QString::number(p.getTiempoServicio())));
    }
}

void MainWindow::delay()
{
    QTime delayTime = QTime::currentTime().addSecs(1);
    while (QTime::currentTime() < delayTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
    }
}

void MainWindow::on_processNumberSP_valueChanged(int arg1)
{
    if (arg1 > 0){
        ui->processAcountPB->setEnabled(true);
    }
    else{
        ui->processAcountPB->setEnabled(false);
    }
}

void MainWindow::on_processAcountPB_clicked()
{
    ui->processNumberSP->setEnabled(false);
    totalProcess = ui->processNumberSP->value();
    generateProcesses();
    ui->stackedWidget->setCurrentIndex(SHOW_PROCESSES);
    setFixedSize(900,580);
}

void MainWindow::on_startRunPB_clicked()
{
    startProcesses();
    ui->startRunPB->setEnabled(false);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_E and state == RUNNING){
        qDebug() << "Interrupcion";
        interrupted = true;
        tT = ACTION_CODE;
    }
    else if (event->key() == Qt::Key_W and state == RUNNING){
        qDebug() << "Error";
        error = true;
        tT = ACTION_CODE;
    }
    else if (event->key() == Qt::Key_P and state == RUNNING){
        qDebug() << "Pause";
        state = PAUSED;
        pause.exec();
    }
    else if (event->key() == Qt::Key_C and state == PAUSED){
        qDebug() << "Continue";
        state = RUNNING;
        pause.quit();
    }
    else{
        qDebug() << "Nothing";
    }
}
