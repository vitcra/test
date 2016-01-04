<?php

$plugin = array(
  'title'        => t('Current topic context'),
  'description'  => t('Provide tokens for current topic.'),
  'context'      => 'swift_framework_create_context_topic',
  'context name' => 'topic_context',
  'keyword'      => 'topic_context',
  'convert list' => 'topic_context_convert_list',
  'convert'      => 'topic_context_convert',
);

function swift_framework_create_context_topic($empty, $data = NULL, $conf = FALSE) {

  $context = new ctools_context('topic_context');
  $context->plugin = 'topic_context';

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if ($currentPage) {
    $topic = $currentPage->getCurrentTopic();
  }
  else {
    $topic = NULL;
  }

  if ($empty || !$topic) {
    return $context;
  }

  if ($topic) {
    $context->data = $topic;
    $context->title = $topic->title;
    $context->argument = $topic->nid;
  }

  return $context;

}

function topic_context_convert_list($plugin) {
  $list = array();

  $entity = entity_get_info('node');
  if (isset($entity['token type'])) {
    $token = $entity['token type'];
  }
  else {
    $token = $plugin['context name'];
  }

  $tokens = token_info();
  if (isset($tokens['tokens'][$token])) {
    foreach ($tokens['tokens'][$token] as $id => $info) {
      if (!isset($list[$id])) {
        $list[$id] = $info['name'];
      }
    }
  }
  return $list;
}

function topic_context_convert($context, $type, $options) {
  $entity_type = 'node';
  $entity = entity_get_info($entity_type);

  if (isset($entity['token type'])) {
    $token = $entity['token type'];
  }
  else {
    $token = $entity_type;
  }

  //$tokens = token_info();

  $values = token_generate($token, array($type => $type), array($token => $context->data), $options);
  if (isset($values[$type])) {
    return $values[$type];
  }
}
