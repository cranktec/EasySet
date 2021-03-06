#include "controller.hpp"

// durchsucht eine Liste aus Karten (p_cards) nach einer ganz bestimmten Karte
Card *Controller::searchForCard(short p_color, short p_shape, short p_number, short p_opacity, Server::Cards &p_cards)
{
    for(auto it = p_cards.begin(); it != p_cards.end(); ++it)
    {
        if((*it)->m_color == p_color && (*it)->m_shape == p_shape && (*it)->m_number == p_number && (*it)->m_opacity == p_opacity)
            return (*it);
    }
    return nullptr; // falls die Karte nicht vorhanden ist, wird der Nullpointer zurückgegeben
}

// Sendet das Packet für die Spielfeldsynchronisation an alle Clients
void Controller::sendFSPacket()
{
    QByteArray packet = m_packetHandler->makeFSPacket(m_field);
    for(auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        std::get<0>(*it)->write(packet);
        std::get<0>(*it)->flush();
    }
    // Methodenaufrufe, um Anzeigen zu synchronisieren
    sendDeckLengthPacket();     
    sendScoreboard();   
    
    // Liefert die Anzahl der auf dem Feld existierenden Sets zurück
    getSetCount();
}

void Controller::sendScorePacket(QTcpSocket *p_socket, short p_score)
{

}

void Controller::sendWaitTimePacket()
{
    QByteArray packet = m_packetHandler->makeWaitTimePacket(m_waitTime);
    for(auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        std::get<0>(*it)->write(packet);
        std::get<0>(*it)->flush();
    }
}

// Sendet die Deckgröße an alle Clients für die Anzeige
void Controller::sendDeckLengthPacket()
{
    QByteArray packet = m_packetHandler->makeDeckLengthPacket(m_deck.size());
    for(auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        std::get<0>(*it)->write(packet);
        std::get<0>(*it)->flush();
    }
}

// sendet das Scoreboard an alle Clients
void Controller::sendScoreboard()
{
    QByteArray scores;
    // schreibt alle Scores in ein QByteArray
    for(auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        scores.append(std::get<1>(*it));
    }
    
    // erstellt aus dem QByteArray ein Packet
    QByteArray packet = m_packetHandler->makeScoresPacket(scores);
    
    //sendet das Packet an alle Clients
    for(auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        std::get<0>(*it)->write(packet);
        std::get<0>(*it)->flush();
    }
}

// prüft, ob die übergebenen Karten ein Set bilden, indem sie wie ein Vektor aufaddiert werden (siehe operator+ in Card)
bool Controller::check(Server::Cards &p_cards)
{
    if(p_cards.size() == 3)
    {
        Card sum(0, 0, 0, 0);
        for(auto it = p_cards.begin(); it != p_cards.end(); ++it)
        {
            sum = sum + **it;
        }
        if(!(sum.m_color + sum.m_number + sum.m_opacity + sum.m_shape))
            return true;
    }
    return false;
}

// liefert einen Client anhand des Sockets
Server::Client &Controller::getClient(QTcpSocket *p_socket)
{
    for(auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if(std::get<0>(*it) == p_socket)
            return *it;
    }
}

// berechnet die Anzahl der Sets auf dem Spielfeld 
short Controller::getSetCount()
{
    short setCount = 0;
    std::list<Card*> checkList;     
    // teste alle möglichen Kombinationen
    for(auto first = m_field.begin(); first != m_field.end(); ++first)
    {
        for(auto second = m_field.begin(); second != m_field.end(); ++second)
        {
            for(auto third = m_field.begin(); third != m_field.end(); ++third)
            {
                checkList.clear();
                // falls die Karten nicht gleich sind
                if(*first != *second && *first != *third && *second != *third)
                {
                    checkList.push_back(*first);
                    checkList.push_back(*second);
                    checkList.push_back(*third);
                    // falls die Karten ein Set bilden wird setCount erhöht
                    if(check(checkList))
                        ++setCount;
                }
            }
        }
    }
    std::cout << "setCount: " << setCount / 6 << std::endl;
    return setCount / 6;    // da jedes Set 6 mal gezählt wird teilen wir durch 6
}

// Konstruktor
Controller::Controller(QObject *p_parent) :
    Server(p_parent),
    m_deck(),
    m_field()
{
    m_extraCards.store(false);
    srand(time(NULL));
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            for(int k = 0; k < 3; k++)
            {
                for(int l = 0; l < 3; l++)
                {
                    m_deck.push_back(new Card(i, j, k, l));
                }
            }
        }
    }
    draw(12);
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(draw()));
    timer->setInterval(m_waitTime);
    timer->start();
}
// ziehe p_count neue Karten vom Stapel und lege sie auf das Feld
void Controller::draw(short p_count)
{
    std::cout << "field_size: " << m_field.size() << std::endl;
    
    // falls bereits zu viele Karten auf dem Feld liegen, wird nichts neues gezogen
    if(m_extraCards.load())
    {
        sendFSPacket();
        return;
    }
    
    // es können maximal so viele Karten gezogen werden, wie welche im Deck liegen
    if(m_deck.size() < p_count)
        p_count = m_deck.size();
    
    // ziehe neue Karten und lege sie aufs Feld
    for(int i = 0; i < p_count; i++)
    {
        int index = rand() % this->m_deck.size();
        auto it = m_deck.begin();
        std::advance(it, index);
        m_field.push_back(std::move(*it));
        m_deck.remove(*it);
    }
    
    if(m_field.size() > 12)
        m_extraCards.store(true);
    else
        m_extraCards.store(false);
    
    // synchronisiere Spielfeld
    sendFSPacket();
}
// verarbeite Spielereingabe
void Controller::retrieveClick(QTcpSocket *p_client, QByteArray p_cards)
{
    Cards cards;
    // Suche angeklickte Karten auf dem Feld zusammen
    for(auto it = p_cards.begin(); it != p_cards.end(); ++it)
    {
        Card *currentCard = searchForCard((*it >> 6) & 0x03, (*it >> 4) & 0x03, (*it >> 2) & 0x03, *it & 0x03, m_field);
        if(currentCard)
            cards.push_back(currentCard);
    }
    // Falls die Karten ein Set bilden, wird das Set vom Spielfeld gelöscht und der Client erhält die Karten und einen Punkt 
    if(check(cards))
    {
        for(auto it = cards.begin(); it != cards.end(); ++it)
        {
            m_field.remove(*it);
        }
        Client &client = getClient(p_client);
        std::get<2>(client).merge(cards);
        std::get<1>(client)++;
        
        // anschließend werden neue Karten nachgelegt
        draw();
        timer->start();
    }
    else
    {

    }
    if(m_field.size() > 12)
        m_extraCards.store(true);
    else
        m_extraCards.store(false);
}
