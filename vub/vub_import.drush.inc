<?php
/**
 * Implements hook_drush_command().
 */
function vub_import_drush_command() {
  $items = array();

  $items['vub-pure-import-items'] = array(
    'description' => 'Batch process queued items',
    'arguments'   => array(
      'limit'    => dt('Batch limit'),
    ),
    'aliases' => array('vubpii'),
  );
  $items['vub-pure-import-tasks'] = array(
    'description' => 'Batch import Task plugins.',
    'options' => array(
      'all' => 'Import all tasks, not just the once since last import',
    ),
    'arguments'   => array(
      'entity'    => dt('Organisation, Person, Publication or Project'),
    ),
    'aliases' => array('vubpit'),
  );

  return $items;
}


/**
 * Drush command to import tasks
 *
 * @param $entity one of Organisation, Person, Publication, Project
 */
function drush_vub_import_vub_pure_import_tasks($entity) {
  importqueue_loadincludes('batch', 'task');

  if (!$entity) {
    drush_set_error('input', 'Which entity would you like to import?');
    return;
  }

  $allowed_tasks = drupal_map_assoc(array('Organisation', 'Person', 'Publication', 'Project'));
  $entity = ucfirst(strtolower(trim($entity)));

  if (empty($allowed_tasks[$entity])) {
    drush_set_error('input', 'The entity requested for import is not valid.');
    return;
  }

  $entity .= 'Task';

  $tasks = array($entity => $entity);

  $batches = importqueue_tasks_batch($tasks, 'admin');

  $all = drush_get_option('all');

  foreach (array_keys($batches['operations']) as $op) {
    $batches['operations'][$op][1][1]['request']['dynamics']['modifiedDate']['fromDate']['updateOnFinish'] = 1;
    if (isset($all)) {
      $batches['operations'][$op][1][1]['request']['arguments']['modifiedDate']['fromDate'] = '';
    }
  }

  //drush_print_r($batches);
  batch_set($batches);
  drush_backend_batch_process();
}

function drush_vub_import_vub_pure_import_items($limit = 10) {

}